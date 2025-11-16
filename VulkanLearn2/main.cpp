#include "pch.h"
#include "main.h"

#include "Core/Window.h"
#include "Core/VulkanSwapchain.h"
#include "Core/VulkanImage.h"

#include "Core/VulkanCommandManager.h"
#include "Core/VulkanPipeline.h"
#include "Core/VulkanSyncManager.h"
#include "Core/VulkanBuffer.h"
#include "Core/VulkanDescriptorManager.h"
#include "Core/VulkanSampler.h"
#include "Core/VulkanDescriptor.h"
#include "Renderer/GeometryPass.h"

#include "Utils/DebugTimer.h"
#include <Scene/RenderObject.h>
#include "Scene/MeshManager.h"
#include "Scene/Model.h"
#include "Scene/TextureManager.h"


// =================================================================================================
// SECTION 1: MAIN ENTRY POINT
// =================================================================================================

/**
 * @brief Điểm vào chính của ứng dụng.
 * Khởi tạo đối tượng Application và bắt đầu vòng lặp chính.
 */
int main()
{
	Application app;
	app.Loop();
}

// =================================================================================================
// SECTION 2: APPLICATION CLASS - PUBLIC INTERFACE (CONSTRUCTOR / DESTRUCTOR / MAIN LOOP)
// =================================================================================================

/**
 * @brief Constructor: Khởi tạo toàn bộ ứng dụng Vulkan.
 * Thiết lập cửa sổ, context Vulkan, swapchain, các chuỗi pipeline render, và tất cả các tài nguyên
 * cần thiết cho việc render (pipelines, buffers, descriptors, scene objects).
 */
Application::Application()
{
	// --- Giai đoạn 1: Khởi tạo Cửa sổ & Vulkan Core ---
	// 1. Tạo cửa sổ
	m_Window = new Window(WINDOW_WIDTH, WINDOW_HEIGHT, "ZOLCOL VULKAN");

	// 2. Khởi tạo các thành phần Vulkan cốt lõi
	m_VulkanContext = new VulkanContext(m_Window->getGLFWWindow(), m_Window->getInstanceExtensionsRequired());
	m_VulkanSwapchain = new VulkanSwapchain(m_VulkanContext->getVulkanHandles(), m_Window->getGLFWWindow());
	m_VulkanCommandManager = new VulkanCommandManager(m_VulkanContext->getVulkanHandles(), MAX_FRAMES_IN_FLIGHT);
	m_VulkanSampler = new VulkanSampler(m_VulkanContext->getVulkanHandles());

	CreateFrameBufferImages();

	// 4. Tạo uniform buffers và descriptors cho từng frame-in-flight
	CreateUniformBuffers();		// Cho RTT pass (Camera)
	CreateMainDescriptors();	// Cho Main pass (Composite)
	CreateBrightDescriptors();	// Cho Bright pass (Trích xuất vùng sáng)
	CreateBlurHDescriptors();	// Cho Blur Horizontal pass
	CreateBlurVDescriptors();	// Cho Blur Vertical pass

	// --- Giai đoạn 3: Tải Scene & Hoàn tất Thiết lập ---
	// 5. Tạo các manager cho scene và tải object
	m_MeshManager = new MeshManager(m_VulkanContext->getVulkanHandles(), m_VulkanCommandManager);
	m_TextureManager = new TextureManager(m_VulkanContext->getVulkanHandles(), m_VulkanCommandManager, m_VulkanSampler->getSampler());

	m_BunnyGirl = new RenderObject("Resources/bunnyGirl.assbin", m_MeshManager, m_TextureManager);
	m_Swimsuit = new RenderObject("Resources/swimSuit.assbin", m_MeshManager, m_TextureManager);
	m_BunnyGirl->SetRotation({ -90, 0, 0 });

	m_RenderObjects.push_back(m_BunnyGirl);
	m_RenderObjects.push_back(m_Swimsuit);

	// Hoàn tất việc tải texture (upload lên GPU) và lấy descriptor texture
	m_TextureManager->FinalizeSetup();

	CreateRenderPasses(); // Initialize GeometryPass

	// 6. Tập hợp tất cả descriptor layouts để tạo pipeline layout
	m_allDescriptors.insert(m_allDescriptors.end(), m_GeometryPass->GetHandles().descriptors.begin(), m_GeometryPass->GetHandles().descriptors.end());
	m_allDescriptors.insert(m_allDescriptors.end(), m_BrightFilterPass->GetHandles().descriptors.begin(), m_BrightFilterPass->GetHandles().descriptors.end());
	m_allDescriptors.insert(m_allDescriptors.end(), m_BlurHPass->GetHandles().descriptors.begin(), m_BlurHPass->GetHandles().descriptors.end());
	m_allDescriptors.insert(m_allDescriptors.end(), m_BlurVPass->GetHandles().descriptors.begin(), m_BlurVPass->GetHandles().descriptors.end());
	m_allDescriptors.insert(m_allDescriptors.end(), m_CompositePass->GetHandles().descriptors.begin(), m_CompositePass->GetHandles().descriptors.end());


	m_VulkanDescriptorManager = new VulkanDescriptorManager(m_VulkanContext->getVulkanHandles(), m_allDescriptors);

	// 8. Tạo các đối tượng đồng bộ (semaphores, fences)
	m_VulkanSyncManager = new VulkanSyncManager(m_VulkanContext->getVulkanHandles(), MAX_FRAMES_IN_FLIGHT, m_VulkanSwapchain->getHandles().swapchainImageCount);

	// 9. Hoàn tất việc tạo buffer cho mesh (upload lên GPU)
	m_MeshManager->CreateBuffers();
}

/**
 * @brief Destructor: Dọn dẹp tất cả tài nguyên đã cấp phát.
 * Phải đảm bảo đợi GPU hoàn thành trước khi giải phóng tài nguyên.
 * Thứ tự dọn dẹp rất quan trọng, thường là ngược lại với thứ tự tạo.
 */
Application::~Application()
{
	// Đợi GPU hoàn thành mọi tác vụ trước khi dọn dẹp
	vkDeviceWaitIdle(m_VulkanContext->getVulkanHandles().device);

	// 1. Pipelines (Phụ thuộc vào DescriptorManager)
	delete(m_GeometryPass);
	delete(m_BrightFilterPass);
	delete(m_BlurVPass);
	delete(m_BlurHPass);
	delete(m_CompositePass);
	// 2. Managers (Quản lý các tài nguyên Vulkan khác)
	delete(m_VulkanDescriptorManager);
	delete(m_VulkanSyncManager);
	delete(m_VulkanCommandManager);
	delete(m_MeshManager);
	delete(m_TextureManager);

	// 3. Buffers (Uniform buffers)
	for (auto& uniformBuffer : m_RTT_UniformBuffers)
	{
		delete(uniformBuffer);
	}

	// 5. Images (Các attachment của Framebuffer)
	for (auto& image : m_TempBlurImages)
	{
		delete(image);
	}
	for (auto& image : m_BrightImages)
	{
		delete(image);
	}
	for (auto& image : m_SceneImages)
	{
		delete(image);
	}
	for (auto& image : m_RTT_ColorImage)
	{
		delete(image);
	}
	for (auto& image : m_RTT_DepthStencilImage)
	{
		delete(image);
	}
	delete(m_Main_DepthStencilImage);
	delete(m_Main_ColorImage);

	// 6. Scene Objects (Phụ thuộc vào Mesh/Texture Managers)
	for (auto& renderObject : m_RenderObjects)
	{
		delete(renderObject);
	}

	// 7. Core Vulkan Objects (Các thành phần cơ bản)
	delete(m_VulkanSampler);

	delete(m_VulkanSwapchain);
	delete(m_VulkanContext); // Context phải được xóa gần cuối

	// 8. Window (Xóa cuối cùng)
	delete(m_Window);
}

/**
 * @brief Vòng lặp chính của ứng dụng.
 * Liên tục xử lý sự kiện cửa sổ và gọi hàm DrawFrame cho đến khi cửa sổ đóng.
 */
void Application::Loop()
{
	while (!m_Window->windowShouldClose())
	{
		m_Window->windowPollEvents();
		DrawFrame();
	}

	// Đợi device rảnh rỗi trước khi thoát
	vkDeviceWaitIdle(m_VulkanContext->getVulkanHandles().device);
}

// =================================================================================================
// SECTION 3: APPLICATION CLASS - CORE FRAME LOGIC (UPDATE & DRAW)
// =================================================================================================

/**
 * @brief Thực hiện tất cả các hoạt động cần thiết để vẽ một frame.
 * Đây là trái tim của vòng lặp render, điều phối việc đồng bộ,
 * cập nhật dữ liệu, ghi command buffer và trình chiếu.
 */
void Application::DrawFrame()
{
	// 1. Đợi fence của frame hiện tại được báo hiệu
	// Đảm bảo rằng frame[m_CurrentFrame] của lần lặp trước đã render xong.
	vkWaitForFences(m_VulkanContext->getVulkanHandles().device, 1, &m_VulkanSyncManager->getCurrentFence(m_CurrentFrame), VK_TRUE, UINT64_MAX);

	// 2. Lấy image có sẵn tiếp theo từ swapchain
	uint32_t imageIndex; // Đây là index của swapchain image *sẽ được vẽ lên*.
	VkResult result = vkAcquireNextImageKHR(m_VulkanContext->getVulkanHandles().device, m_VulkanSwapchain->getHandles().swapchain, UINT64_MAX,
		m_VulkanSyncManager->getCurrentImageAvailableSemaphore(m_CurrentFrame), // Báo hiệu semaphore này khi image sẵn sàng
		VK_NULL_HANDLE, &imageIndex);

	// Xử lý trường hợp swapchain không còn tối ưu (ví dụ: cửa sổ bị resize)
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		// TODO: Tạo lại swapchain
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("FAILED TO ACQUIRE SWAPCHAIN IMAGE");
	}

	// 3. Reset fence về trạng thái chưa được báo hiệu (vì chúng ta sắp dùng lại nó)
	vkResetFences(m_VulkanContext->getVulkanHandles().device, 1, &m_VulkanSyncManager->getCurrentFence(m_CurrentFrame));

	// 4. Cập nhật dữ liệu động cho frame hiện tại
	UpdateRTT_Uniforms(); // Cập nhật camera
	UpdateRenderObjectTransforms(); // Cập nhật vị trí/xoay của model

	// 5. Ghi command buffer để vẽ
	vkResetCommandBuffer(m_VulkanCommandManager->getHandles().commandBuffers[m_CurrentFrame], 0);
	RecordCommandBuffer(m_VulkanCommandManager->getHandles().commandBuffers[m_CurrentFrame], imageIndex);

	// 6. Submit command buffer vào hàng đợi graphics
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_VulkanCommandManager->getHandles().commandBuffers[m_CurrentFrame];

	// Đợi semaphore 'image available' trước khi bắt đầu
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_VulkanSyncManager->getCurrentImageAvailableSemaphore(m_CurrentFrame);
	submitInfo.pWaitDstStageMask = &waitStage;

	// Báo hiệu semaphore 'render finished' khi hoàn thành
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_VulkanSyncManager->getCurrentRenderFinishedSemaphore(imageIndex);

	// Sử dụng fence để CPU biết khi nào GPU hoàn thành
	VK_CHECK(vkQueueSubmit(m_VulkanContext->getVulkanHandles().graphicQueue, 1, &submitInfo, m_VulkanSyncManager->getCurrentFence(m_CurrentFrame)),
		"FAILED TO SUBMIT COMMAND BUFFER");

	// 7. Trình chiếu (present) image đã render ra màn hình
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_VulkanSwapchain->getHandles().swapchain;

	// Đợi semaphore 'render finished' trước khi trình chiếu
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_VulkanSyncManager->getCurrentRenderFinishedSemaphore(imageIndex);
	presentInfo.pImageIndices = &imageIndex; // Chỉ định swapchain image nào sẽ được present

	result = vkQueuePresentKHR(m_VulkanContext->getVulkanHandles().presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		// TODO: Tạo lại swapchain
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("FAILED TO PRESENT SWAPCHAIN IMAGE");
	}

	// 8. Chuyển sang index của frame tiếp theo
	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

/**
 * @brief Cập nhật Uniform Buffer Object (UBO) với ma trận camera hiện tại.
 * Dữ liệu này được sử dụng trong RTT (Scene) render pass.
 */
void Application::UpdateRTT_Uniforms()
{
	// Thiết lập view camera (nhìn từ vị trí (0, 3, 4) vào (0, 2, 0))
	m_RTT_Ubo.view = glm::lookAt(glm::vec3(0.0f, 3.0f, 4.0f), glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	// Thiết lập ma trận projection (phối cảnh)
	m_RTT_Ubo.proj = glm::perspective(glm::radians(45.0f), m_VulkanSwapchain->getHandles().swapChainExtent.width / (float)m_VulkanSwapchain->getHandles().swapChainExtent.height, 0.1f, 10.0f);

	// Sửa lỗi tọa độ Y của Vulkan (ngược với OpenGL)
	m_RTT_Ubo.proj[1][1] *= -1;

	// Tải dữ liệu UBO lên buffer của GPU cho frame hiện tại
	m_RTT_UniformBuffers[m_CurrentFrame]->UploadData(&m_RTT_Ubo, sizeof(m_RTT_Ubo), 0);
}

/**
 * @brief Cập nhật transform (vị trí, xoay, tỷ lệ) của các object trong scene.
 * Sử dụng push constants để gửi ma trận model cho từng object.
 */
void Application::UpdateRenderObjectTransforms()
{
	// Animation đơn giản dựa trên thời gian
	static auto startTime = std::chrono::high_resolution_clock::now();
	static auto lastTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
	lastTime = currentTime;

	m_BunnyGirl->SetPosition({ 1, 0, 0 });
	m_BunnyGirl->Rotate(glm::vec3(0, deltaTime * 45.0f, 0));
	m_BunnyGirl->Scale({ 0.05f, 0.05f, 0.05f });

	m_Swimsuit->SetPosition({ -1, 0, 0 });
	m_Swimsuit->Scale({ 0.05f, 0.05f, 0.05f });
	m_Swimsuit->Rotate(glm::vec3(0, deltaTime * -45.0f, 0));
}

// =================================================================================================
// SECTION 4: APPLICATION CLASS - DRAWING & COMMAND BUFFER RECORDING
// =================================================================================================

/**
 * @brief Ghi lại tất cả các lệnh render vào một command buffer.
 * Đây là chuỗi render (post-processing pipeline):
 * 1. RTT Pass: Render scene 3D vào một texture (m_SceneImages).
 * 2. Bright Pass: Lọc ra các vùng sáng từ scene texture (-> m_BrightImages).
 * 3. BlurH Pass: Làm mờ ngang vùng sáng (-> m_TempBlurImages).
 * 4. BlurV Pass: Làm mờ dọc kết quả (-> m_BrightImages, ghi đè).
 * 5. Main Pass: Tổng hợp (composite) scene texture gốc và texture đã làm mờ (bloom)
 * lên swapchain image để hiển thị.
 */
void Application::RecordCommandBuffer(const VkCommandBuffer& cmdBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo cmdBeginInfo{};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmdBeginInfo), "FAILED TO BEGIN COMMAND BUFFER");

	// Chuỗi các render pass
	m_GeometryPass->Execute(&cmdBuffer, imageIndex, m_CurrentFrame);
	m_BrightFilterPass->Execute(&cmdBuffer, imageIndex, m_CurrentFrame);
	m_BlurHPass->Execute(&cmdBuffer, imageIndex, m_CurrentFrame);
	m_BlurVPass->Execute(&cmdBuffer, imageIndex, m_CurrentFrame);
	m_CompositePass->Execute(&cmdBuffer, imageIndex, m_CurrentFrame);

	VK_CHECK(vkEndCommandBuffer(cmdBuffer), "FAILED TO END COMMAND BUFFER");
}


// --- 4.3: Blur Horizontal Pass ---

/**
 * @brief Ghi lệnh cho bước render Blur Horizontal.
 * Input: m_BrightImages[m_CurrentFrame].
 * Output: m_TempBlurImages[m_CurrentFrame].
 */

// --- 4.4: Blur Vertical Pass ---


// =================================================================================================
// SECTION 5: APPLICATION CLASS - INITIALIZATION HELPERS
// =================================================================================================

/**
 * @brief Tạo tất cả các VulkanImage sẽ được dùng làm attachment cho các bước render.
 */
void Application::CreateFrameBufferImages()
{
	VkExtent2D swapchainExtent = m_VulkanSwapchain->getHandles().swapChainExtent;
	VkFormat swapchainFormat = m_VulkanSwapchain->getHandles().swapchainSupportDetails.chosenFormat.format;

	// --- Images cho RTT Pass (Vẽ scene 3D) ---
	// 1. RTT Color Image (MSAA)
	VulkanImageCreateInfo RTT_ColorImageInfo{};
	RTT_ColorImageInfo.width = swapchainExtent.width;
	RTT_ColorImageInfo.height = swapchainExtent.height;
	RTT_ColorImageInfo.mipLevels = 1;
	RTT_ColorImageInfo.samples = MSAA_SAMPLES;
	RTT_ColorImageInfo.format = swapchainFormat;
	RTT_ColorImageInfo.imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	RTT_ColorImageInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VulkanImageViewCreateInfo RTT_ColorImageViewInfo{};
	RTT_ColorImageViewInfo.format = swapchainFormat;
	RTT_ColorImageViewInfo.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	RTT_ColorImageViewInfo.mipLevels = 1;

	m_RTT_ColorImage.resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_RTT_ColorImage[i] = new VulkanImage(m_VulkanContext->getVulkanHandles(), RTT_ColorImageInfo, RTT_ColorImageViewInfo);
	}

	// 2. RTT Depth/Stencil Image (MSAA)
	VulkanImageCreateInfo RTT_DepthImageInfo{};
	RTT_DepthImageInfo.width = swapchainExtent.width;
	RTT_DepthImageInfo.height = swapchainExtent.height;
	RTT_DepthImageInfo.mipLevels = 1;
	RTT_DepthImageInfo.samples = MSAA_SAMPLES;
	RTT_DepthImageInfo.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
	RTT_DepthImageInfo.imageUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	RTT_DepthImageInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VulkanImageViewCreateInfo RTT_DepthImageViewInfo{};
	RTT_DepthImageViewInfo.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
	RTT_DepthImageViewInfo.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	RTT_DepthImageViewInfo.mipLevels = 1;

	m_RTT_DepthStencilImage.resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_RTT_DepthStencilImage[i] = new VulkanImage(m_VulkanContext->getVulkanHandles(), RTT_DepthImageInfo, RTT_DepthImageViewInfo);
	}

	// 3. m_SceneImages (Resolve Target cho RTT, Input cho Post-Processing)
	m_SceneImages.resize(MAX_FRAMES_IN_FLIGHT);
	VulkanImageCreateInfo SceneImageInfo{};
	SceneImageInfo.width = swapchainExtent.width;
	SceneImageInfo.height = swapchainExtent.height;
	SceneImageInfo.mipLevels = 1;
	SceneImageInfo.samples = VK_SAMPLE_COUNT_1_BIT; // Không MSAA
	SceneImageInfo.format = swapchainFormat;
	SceneImageInfo.imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; // Dùng làm attachment và sampler
	SceneImageInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VulkanImageViewCreateInfo SceneImageViewInfo{};
	SceneImageViewInfo.format = swapchainFormat;
	SceneImageViewInfo.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	SceneImageViewInfo.mipLevels = 1;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_SceneImages[i] = new VulkanImage(m_VulkanContext->getVulkanHandles(), SceneImageInfo, SceneImageViewInfo);
	}

	// --- Images cho Main Pass (Vẽ ra Swapchain) ---
	// 4. Main Color Image (MSAA)
	m_Main_ColorImage = new VulkanImage(m_VulkanContext->getVulkanHandles(), RTT_ColorImageInfo, RTT_ColorImageViewInfo);
	// 5. Main Depth/Stencil Image (MSAA)
	m_Main_DepthStencilImage = new VulkanImage(m_VulkanContext->getVulkanHandles(), RTT_DepthImageInfo, RTT_DepthImageViewInfo);

	// --- Images cho Post-Processing (Bloom) ---
	// 6. m_BrightImages (Output của Bright, Output của BlurV, Input của BlurH, Input của Main)
	m_BrightImages.resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_BrightImages[i] = new VulkanImage(m_VulkanContext->getVulkanHandles(), SceneImageInfo, SceneImageViewInfo);
	}

	// 7. m_TempBlurImages (Output của BlurH, Input của BlurV)
	m_TempBlurImages.resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_TempBlurImages[i] = new VulkanImage(m_VulkanContext->getVulkanHandles(), SceneImageInfo, SceneImageViewInfo);
	}
}

void Application::CreateRenderPasses()
{
	GeometryPassCreateInfo geometryInfo{};
	geometryInfo.textureManager = m_TextureManager;
	geometryInfo.meshManager = m_MeshManager;
	geometryInfo.uboDescriptors = &m_RTT_UniformDescriptors;
	geometryInfo.colorImages = &m_RTT_ColorImage;
	geometryInfo.depthStencilImages = &m_RTT_DepthStencilImage;
	geometryInfo.outputImage = &m_SceneImages;
	geometryInfo.BackgroundColor = BACKGROUND_COLOR;
	geometryInfo.vulkanSwapchainHandles = &m_VulkanSwapchain->getHandles();
	geometryInfo.renderObjects = &m_RenderObjects;
	geometryInfo.pushConstantData = &m_PushConstantData;
	geometryInfo.vulkanHandles = &m_VulkanContext->getVulkanHandles();
	geometryInfo.MSAA_SAMPLES = MSAA_SAMPLES;
	geometryInfo.fragShaderFilePath = "Shaders/RTT_Shader.frag.spv";
	geometryInfo.vertShaderFilePath = "Shaders/RTT_Shader.vert.spv";

	m_GeometryPass = new GeometryPass(geometryInfo);

	BrightFilterPassCreateInfo brightFilterInfo{};
	brightFilterInfo.vulkanHandles = &m_VulkanContext->getVulkanHandles();
	brightFilterInfo.vulkanSwapchainHandles = &m_VulkanSwapchain->getHandles();
	brightFilterInfo.MSAA_SAMPLES = VK_SAMPLE_COUNT_1_BIT; // Post-processing passes don't need MSAA
	brightFilterInfo.MAX_FRAMES_IN_FLIGHT = MAX_FRAMES_IN_FLIGHT;
	brightFilterInfo.textureDescriptors = &m_BrightTextureDescriptors;
	brightFilterInfo.fragShaderFilePath = "Shaders/Bright_Shader.frag.spv";
	brightFilterInfo.vertShaderFilePath = "Shaders/Main_Shader.vert.spv";
	brightFilterInfo.BackgroundColor = BACKGROUND_COLOR;
	brightFilterInfo.outputImage = &m_BrightImages;

	m_BrightFilterPass = new BrightFilterPass(brightFilterInfo);

	BlurPassCreateInfo blurHInfo{};
	blurHInfo.vulkanHandles = &m_VulkanContext->getVulkanHandles();
	blurHInfo.vulkanSwapchainHandles = &m_VulkanSwapchain->getHandles();
	blurHInfo.MSAA_SAMPLES = VK_SAMPLE_COUNT_1_BIT;
	blurHInfo.inputTextureDescriptors = &m_BlurHTextureDescriptors;
	blurHInfo.fragShaderFilePath = "Shaders/BlurH_Shader.frag.spv";
	blurHInfo.vertShaderFilePath = "Shaders/Main_Shader.vert.spv";
	blurHInfo.BackgroundColor = BACKGROUND_COLOR;
	blurHInfo.outputImages = &m_TempBlurImages;

	m_BlurHPass = new BlurPass(blurHInfo);

	BlurPassCreateInfo blurVInfo{};
	blurVInfo.vulkanHandles = &m_VulkanContext->getVulkanHandles();
	blurVInfo.vulkanSwapchainHandles = &m_VulkanSwapchain->getHandles();
	blurVInfo.MSAA_SAMPLES = VK_SAMPLE_COUNT_1_BIT;
	blurVInfo.inputTextureDescriptors = &m_BlurVTextureDescriptors;
	blurVInfo.fragShaderFilePath = "Shaders/BlurV_Shader.frag.spv";
	blurVInfo.vertShaderFilePath = "Shaders/Main_Shader.vert.spv";
	blurVInfo.BackgroundColor = BACKGROUND_COLOR;
	blurVInfo.outputImages = &m_BrightImages;

	m_BlurVPass = new BlurPass(blurVInfo);

	CompositePassCreateInfo compositeInfo{};
	compositeInfo.vulkanHandles = &m_VulkanContext->getVulkanHandles();
	compositeInfo.vulkanSwapchainHandles = &m_VulkanSwapchain->getHandles();
	compositeInfo.MSAA_SAMPLES = MSAA_SAMPLES; // Main pass uses MSAA for resolve
	compositeInfo.textureDescriptors = &m_MainTextureDescriptors;
	compositeInfo.fragShaderFilePath = "Shaders/Main_Shader.frag.spv";
	compositeInfo.vertShaderFilePath = "Shaders/Main_Shader.vert.spv";
	compositeInfo.BackgroundColor = BACKGROUND_COLOR;
	compositeInfo.mainColorImage = m_Main_ColorImage;
	compositeInfo.mainDepthStencilImage = m_Main_DepthStencilImage;

	m_CompositePass = new CompositePass(compositeInfo);


}

// --- 5.2: Buffers & Descriptors ---

/**
 * @brief Tạo uniform buffers (UBO) cho RTT pass.
 * Các buffer này chứa ma trận View-Projection (camera).
 * Tạo một UBO cho mỗi frame-in-flight.
 */
void Application::CreateUniformBuffers()
{
	m_RTT_UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	m_RTT_UniformDescriptors.resize(MAX_FRAMES_IN_FLIGHT);

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.queueFamilyIndexCount = 1;
	bufferInfo.pQueueFamilyIndices = &m_VulkanContext->getVulkanHandles().queueFamilyIndices.GraphicQueueIndex;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.size = sizeof(UniformBufferObject);
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		// 1. Tạo buffer (với VMA_MEMORY_USAGE_CPU_TO_GPU để có thể map/unmap)
		m_RTT_UniformBuffers[i] = new VulkanBuffer(m_VulkanContext->getVulkanHandles(), m_VulkanCommandManager, bufferInfo, VMA_MEMORY_USAGE_CPU_TO_GPU);

		// 2. Định nghĩa thông tin binding cho UBO
		BindingElementInfo uniformElementInfo;
		uniformElementInfo.binding = 0; // layout(binding = 0)
		uniformElementInfo.pImmutableSamplers = nullptr;
		uniformElementInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformElementInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Dùng trong Vertex Shader
		uniformElementInfo.descriptorCount = 1;

		VkDescriptorBufferInfo uniformBufferInfo;
		uniformBufferInfo.buffer = m_RTT_UniformBuffers[i]->GetHandles().buffer;
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = m_RTT_UniformBuffers[i]->GetHandles().bufferSize;

		std::vector<VkDescriptorBufferInfo> uniformBufferInfos = { uniformBufferInfo };

		BufferDescriptorUpdateInfo uniformBufferUpdate{};
		uniformBufferUpdate.binding = 0;
		uniformBufferUpdate.firstArrayElement = 0;
		uniformBufferUpdate.bufferInfos = uniformBufferInfos;

		uniformElementInfo.bufferDescriptorUpdateInfoCount = 1;
		uniformElementInfo.pBufferDescriptorUpdates = &uniformBufferUpdate;

		// 3. Tạo descriptor (set)
		std::vector<BindingElementInfo> uniformBindings{ uniformElementInfo };
		m_RTT_UniformDescriptors[i] = new VulkanDescriptor(m_VulkanContext->getVulkanHandles(), uniformBindings, 1); // Set 1

		// 4. Thêm descriptor vào danh sách để tạo pipeline layout
	}
}

/**
 * @brief Tạo descriptors cho Main (composite) pass.
 * Descriptor set này chứa 2 texture:
 * Binding 0: m_SceneImages (Scene gốc)
 * Binding 1: m_BrightImages (Đã blur, hiệu ứng bloom)
 */
void Application::CreateMainDescriptors()
{
	m_MainTextureDescriptors.resize(MAX_FRAMES_IN_FLIGHT);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		// --- Binding 0: Scene Texture ---
		BindingElementInfo bindingElement{};
		bindingElement.binding = 0;
		bindingElement.descriptorCount = 1;
		bindingElement.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindingElement.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_SceneImages[i]->GetHandles().imageView;
		imageInfo.sampler = m_VulkanSampler->getSampler();

		ImageDescriptorUpdateInfo imageUpdateInfo{};
		imageUpdateInfo.binding = 0;
		imageUpdateInfo.firstArrayElement = 0;
		std::vector<VkDescriptorImageInfo> imageInfos{ imageInfo };
		imageUpdateInfo.imageInfos = imageInfos;

		bindingElement.imageDescriptorUpdateInfoCount = 1;
		bindingElement.pImageDescriptorUpdates = &imageUpdateInfo;


		// --- Binding 1: Bright (Bloom) Texture ---
		BindingElementInfo bindingElement2{};
		bindingElement2.binding = 1;
		bindingElement2.descriptorCount = 1;
		bindingElement2.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindingElement2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorImageInfo imageInfo2{};
		imageInfo2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo2.imageView = m_BrightImages[i]->GetHandles().imageView;
		imageInfo2.sampler = m_VulkanSampler->getSampler();

		ImageDescriptorUpdateInfo imageUpdateInfo2{};
		imageUpdateInfo2.binding = 1;
		imageUpdateInfo2.firstArrayElement = 0;
		std::vector<VkDescriptorImageInfo> imageInfos2{ imageInfo2 };
		imageUpdateInfo2.imageInfos = imageInfos2;

		bindingElement2.imageDescriptorUpdateInfoCount = 1;
		bindingElement2.pImageDescriptorUpdates = &imageUpdateInfo2;

		// Tạo descriptor
		std::vector<BindingElementInfo> bindingElements = { bindingElement ,bindingElement2 };
		m_MainTextureDescriptors[i] = new VulkanDescriptor(m_VulkanContext->getVulkanHandles(), bindingElements, 0); // Set 0
	}
}

/**
 * @brief Tạo descriptors cho Bright pass.
 * Descriptor set này chứa 1 texture:
 * Binding 0: m_SceneImages (Input)
 */
void Application::CreateBrightDescriptors()
{
	m_BrightTextureDescriptors.resize(MAX_FRAMES_IN_FLIGHT);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		// --- Binding 0: Scene Texture ---
		BindingElementInfo bindingElement{};
		bindingElement.binding = 0;
		bindingElement.descriptorCount = 1;
		bindingElement.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindingElement.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_SceneImages[i]->GetHandles().imageView;
		imageInfo.sampler = m_VulkanSampler->getSampler();

		ImageDescriptorUpdateInfo imageUpdateInfo{};
		imageUpdateInfo.binding = 0;
		imageUpdateInfo.firstArrayElement = 0;
		std::vector<VkDescriptorImageInfo> imageInfos{ imageInfo };
		imageUpdateInfo.imageInfos = imageInfos;

		bindingElement.imageDescriptorUpdateInfoCount = 1;
		bindingElement.pImageDescriptorUpdates = &imageUpdateInfo;

		std::vector<BindingElementInfo> bindingElements = { bindingElement };

		m_BrightTextureDescriptors[i] = new VulkanDescriptor(m_VulkanContext->getVulkanHandles(), bindingElements, 0); // Set 0
	}
}

/**
 * @brief Tạo descriptors cho Blur Horizontal pass.
 * Descriptor set này chứa 1 texture:
 * Binding 0: m_BrightImages (Input, output từ Bright pass)
 */
void Application::CreateBlurHDescriptors()
{
	m_BlurHTextureDescriptors.resize(MAX_FRAMES_IN_FLIGHT);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		// --- Binding 0: Bright Texture ---
		BindingElementInfo bindingElement{};
		bindingElement.binding = 0;
		bindingElement.descriptorCount = 1;
		bindingElement.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindingElement.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_BrightImages[i]->GetHandles().imageView;
		imageInfo.sampler = m_VulkanSampler->getSampler();

		ImageDescriptorUpdateInfo imageUpdateInfo{};
		imageUpdateInfo.binding = 0;
		imageUpdateInfo.firstArrayElement = 0;
		std::vector<VkDescriptorImageInfo> imageInfos = { imageInfo };
		imageUpdateInfo.imageInfos = imageInfos;

		bindingElement.imageDescriptorUpdateInfoCount = 1;
		bindingElement.pImageDescriptorUpdates = &imageUpdateInfo;

		std::vector<BindingElementInfo> bindingElements = { bindingElement };

		m_BlurHTextureDescriptors[i] = new VulkanDescriptor(m_VulkanContext->getVulkanHandles(), bindingElements, 0); // Set 0
	}
}

/**
 * @brief Tạo descriptors cho Blur Vertical pass.
 * Descriptor set này chứa 1 texture:
 * Binding 0: m_TempBlurImages (Input, output từ BlurH pass)
 */
void Application::CreateBlurVDescriptors()
{
	m_BlurVTextureDescriptors.resize(MAX_FRAMES_IN_FLIGHT);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		// --- Binding 0: Temp (H-Blurred) Texture ---
		BindingElementInfo bindingElement{};
		bindingElement.binding = 0;
		bindingElement.descriptorCount = 1;
		bindingElement.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindingElement.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_TempBlurImages[i]->GetHandles().imageView;
		imageInfo.sampler = m_VulkanSampler->getSampler();

		ImageDescriptorUpdateInfo imageUpdateInfo{};
		imageUpdateInfo.binding = 0;
		imageUpdateInfo.firstArrayElement = 0;
		std::vector<VkDescriptorImageInfo> imageInfos = { imageInfo };
		imageUpdateInfo.imageInfos = imageInfos;

		bindingElement.imageDescriptorUpdateInfoCount = 1;
		bindingElement.pImageDescriptorUpdates = &imageUpdateInfo;

		std::vector<BindingElementInfo> bindingElements = { bindingElement };

		m_BlurVTextureDescriptors[i] = new VulkanDescriptor(m_VulkanContext->getVulkanHandles(), bindingElements, 0); // Set 0
	}
}
