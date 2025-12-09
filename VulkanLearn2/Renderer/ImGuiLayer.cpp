#include "pch.h"
#include "ImGuiLayer.h"
#include "Core/Window.h"
#include "Core/VulkanSwapchain.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "Core/GameTime.h"
#include "Core/VulkanImage.h"
#include "Core/VulkanSampler.h"
#include "Core/Input.h"

ImGuiLayer::ImGuiLayer(Window* window, const VulkanHandles& vulkanHandles, const VulkanSwapchain* vulkanSwapchain, 
	const std::vector<VulkanImage*>* sceneImage, const VulkanSampler* vulkanSampler):
	m_VulkanHandles(vulkanHandles),
	m_VulkanSwapchain(vulkanSwapchain)
{
	Init(window, vulkanHandles, vulkanSwapchain);
	RegisterSceneImage(sceneImage, vulkanSampler);
}

ImGuiLayer::~ImGuiLayer()
{
	ImGui_ImplGlfw_Shutdown();
	ImGui_ImplVulkan_Shutdown();
	
	ImGui::DestroyContext();
	vkDestroyDescriptorPool(m_VulkanHandles.device, m_ImGuiDescriptorPool, nullptr);
}

void ImGuiLayer::BeginFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// 2. --- THIẾT LẬP DOCKSPACE ---
	// Mục tiêu: Tạo một cửa sổ vô hình bao trùm toàn bộ màn hình để làm "cha"

	// Lấy thông tin màn hình chính (Viewport)
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	// Đặt vị trí và kích thước cửa sổ này bằng đúng Viewport
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);

	// Cài đặt style đặc biệt: Không bo tròn, không viền
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	// Cài đặt cờ cho cửa sổ DockSpace:
	// - NoDocking: Bản thân cái nền này không được dock vào đâu cả
	// - NoTitleBar, NoCollapse, NoResize, NoMove: Cố định hoàn toàn
	// - NoBringToFrontOnFocus: Luôn nằm dưới cùng
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	// Loại bỏ Padding để các cửa sổ con dính sát mép màn hình
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	// Bắt đầu vẽ cửa sổ DockSpace (Nó là một cửa sổ ImGui bình thường nhưng được config đặc biệt)
	ImGui::Begin("Zolcol Engine DockSpace", nullptr, window_flags);

	// Trả lại các biến Style đã Push ở trên (Padding, Rounding, BorderSize)
	ImGui::PopStyleVar(3);

	// 3. --- KÍCH HOẠT DOCKSPACE ID ---
	ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");

	// Đây là hàm phép thuật cho phép bạn kéo thả cửa sổ vào vùng này
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

	ImGui::End();
}

void ImGuiLayer::RenderFrame(VkCommandBuffer cmdBuffer, uint32_t imageIndex, uint32_t currentFrame)
{
	// 1. Code vẽ UI của bạn ở đây
	ImGui::ShowDemoWindow(); // <--- Cửa sổ Demo có sẵn để test

	// Thử tạo một cửa sổ riêng
	ImGui::Begin("Engine Stats");
	ImGui::Text("Hello from Zolcol Engine!");
	ImGui::Text("FPS: %.1f", 1.0f / Core::Time::GetDeltaTime());
	ImGui::End();

	// Cửa sổ Scene Viewport
	//ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar;
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });

	if (ImGui::Begin("Scene ViewPort", nullptr, windowFlags))
	{
		if (!m_SceneImageDescriptorSet.empty() && currentFrame < m_SceneImageDescriptorSet.size())
		{
			Input::hoverOnSceneViewPort = ImGui::IsWindowHovered();
			Input::focusOnSceneViewPort = ImGui::IsWindowFocused();

			VkDescriptorSet currentSceneImageDS = m_SceneImageDescriptorSet[currentFrame];

			ImVec2 imageSize = ImGui::GetContentRegionAvail();

			ImGui::Image((ImTextureRef)(currentSceneImageDS), imageSize);
		}
		else
		{
			ImGui::Text("Texture not loaded or invalid index!");
		}
	}
	else
	{
		ImGui::Text("Cant Begin Scene Viewport Window");
	}
	ImGui::End();

	ImGui::PopStyleVar();

	// 2. Render Data
	ImGui::Render();

	// 3. Ghi lệnh vẽ vào Command Buffer
	RecordDrawCommandBuffer(cmdBuffer, imageIndex);
}



void ImGuiLayer::Init(Window* window, const VulkanHandles& vulkanHandles, const VulkanSwapchain* vulkanSwapchain)
{
	// 1. Tạo Descriptor Pool riêng (ImGui cần cái này để chứa font texture)
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VK_CHECK(vkCreateDescriptorPool(vulkanHandles.device, &pool_info, nullptr, &m_ImGuiDescriptorPool), "Lỗi tạo ImGui Pool");

	// 2. Setup Context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Cho phép dùng bàn phím
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // <--- QUAN TRỌNG: Bật Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark(); // Giao diện tối

	// 3. Init Backend (GLFW)
	ImGui_ImplGlfw_InitForVulkan(window->getGLFWWindow(), true);

	// 4. Init Backend (Vulkan) với Dynamic Rendering
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = vulkanHandles.instance;
	init_info.PhysicalDevice = vulkanHandles.physicalDevice;
	init_info.Device = vulkanHandles.device;
	init_info.QueueFamily = vulkanHandles.queueFamilyIndices.GraphicQueueIndex;
	init_info.Queue = vulkanHandles.graphicQueue;
	init_info.DescriptorPool = m_ImGuiDescriptorPool;
	init_info.MinImageCount = vulkanSwapchain->getHandles().swapchainImageCount;
	init_info.ImageCount = vulkanSwapchain->getHandles().swapchainImageCount;

	// --- CẤU HÌNH DYNAMIC RENDERING ---
	init_info.UseDynamicRendering = true;

	// Lấy format màu từ Swapchain hiện tại
	const VkFormat color_attachment_formats[] = { m_VulkanSwapchain->getHandles().swapchainSupportDetails.chosenFormat.format };

	VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info = {};
	pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	pipeline_rendering_create_info.colorAttachmentCount = 1;
	pipeline_rendering_create_info.pColorAttachmentFormats = color_attachment_formats;
	// ImGui vẽ đè lên swapchain nên không cần Depth/Stencil
	pipeline_rendering_create_info.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
	pipeline_rendering_create_info.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

	init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo = pipeline_rendering_create_info;

	ImGui_ImplVulkan_Init(&init_info);
}

void ImGuiLayer::RegisterSceneImage(const std::vector<VulkanImage*>* sceneImage, const VulkanSampler* vulkanSampler)
{
	m_SceneImageDescriptorSet.resize(sceneImage->size());

	for (size_t i = 0; i < m_SceneImageDescriptorSet.size(); i++)
	{
		m_SceneImageDescriptorSet[i] = ImGui_ImplVulkan_AddTexture(
			vulkanSampler->getSampler(),
			(*sceneImage)[i]->GetHandles().imageView,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}

void ImGuiLayer::RecordDrawCommandBuffer(VkCommandBuffer cmdBuffer, uint32_t imageIndex)
{
	// Engine của bạn vừa vẽ xong Composite Pass, ảnh đang ở layout PRESENT_SRC_KHR.
	// ImGui cần COLOR_ATTACHMENT_OPTIMAL để vẽ lên.

	VkImage swapchainImage = m_VulkanSwapchain->getHandles().swapchainImages[imageIndex];

	// Chuyển layout: Present -> Color Attachment
	VulkanImage::TransitionLayout(cmdBuffer, swapchainImage, 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0, 1);

	VkRenderingAttachmentInfo colorAttachment = {};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.imageView = m_VulkanSwapchain->getHandles().swapchainImageViews[imageIndex];
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // LOAD: Giữ nguyên hình ảnh game cũ
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // STORE: Lưu kết quả có UI

	VkRenderingInfo renderingInfo = {};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.renderArea = { {0, 0}, m_VulkanSwapchain->getHandles().swapChainExtent };
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;

	vkCmdBeginRendering(cmdBuffer, &renderingInfo);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
	vkCmdEndRendering(cmdBuffer);

	// Chuyển layout: Color Attachment -> Present (trả về trạng thái cũ để trình chiếu)
	VulkanImage::TransitionLayout(cmdBuffer, swapchainImage, 1,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0,
		0, 1);
}

void ImGuiLayer::UpdateViewports()
{
	// Cập nhật và vẽ các cửa sổ phụ (Viewports)
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}
