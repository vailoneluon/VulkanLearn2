#include "pch.h"
#include "main.h"

#include "Core/Window.h"
#include "Core/VulkanContext.h"
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
#include "Renderer/BrightFilterPass.h"
#include "Renderer/CompositePass.h"
#include "Renderer/BlurPass.h"
#include "Renderer/LightingPass.h"
#include "Utils/DebugTimer.h"
#include "Utils/ModelLoader.h"
#include "Scene/MeshManager.h"
#include "Scene/Model.h"
#include "Scene/TextureManager.h"
#include "Scene/MaterialManager.h"
#include "Scene\LightManager.h"
#include "Renderer/ShadowMapPass.h"
#include "Scene/Scene.h"
#include "Scene/Component.h"
#include "Scene/TransformSystem.h"




// =================================================================================================
// SECTION 1: ĐIỂM VÀO CHÍNH (MAIN ENTRY POINT)
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
// SECTION 2: LỚP APPLICATION - GIAO DIỆN CÔNG KHAI (CONSTRUCTOR / DESTRUCTOR / MAIN LOOP) 
// =================================================================================================

/**
 * @brief Constructor: Khởi tạo toàn bộ ứng dụng Vulkan.
 * Tuần tự thực hiện các bước thiết lập cốt lõi:
 * 1. Tạo cửa sổ và các thành phần Vulkan cơ bản (Context, Swapchain, Sampler...).
 * 2. Tạo các tài nguyên framebuffer (ảnh attachments) cho các render pass.
 * 3. Tạo uniform buffers cho dữ liệu shader (ví dụ: camera).
 * 4. Tải dữ liệu scene (model, texture).
 * 5. Tạo các render pass (Geometry, Post-processing).
 * 6. Hoàn tất việc thiết lập descriptors và các đối tượng đồng bộ hóa.
 * 7. Tải dữ liệu hình học (mesh) lên GPU.
 */
Application::Application()
{
	// --- 1. KHỞI TẠO CÁC THÀNH PHẦN CỐT LÕI ---
	m_Window = new Window(WINDOW_WIDTH, WINDOW_HEIGHT, "ZOLCOL VULKAN");
	m_VulkanContext = new VulkanContext(m_Window->getGLFWWindow(), m_Window->getInstanceExtensionsRequired());
	m_VulkanSwapchain = new VulkanSwapchain(m_VulkanContext->getVulkanHandles(), m_Window->getGLFWWindow());
	m_VulkanCommandManager = new VulkanCommandManager(m_VulkanContext->getVulkanHandles(), MAX_FRAMES_IN_FLIGHT);
	m_VulkanSampler = new VulkanSampler(m_VulkanContext->getVulkanHandles());
	m_Scene = new Scene();

	CreateSceneLights();

	// --- 2. TẠO TÀI NGUYÊN FRAMEBUFFER (ATTACHMENTS) ---
	// Tạo các ảnh (VulkanImage) sẽ được dùng làm đầu ra cho các render pass.
	CreateFrameBufferImages();

	// --- 3. TẠO TÀI NGUYÊN CHO SHADER ---
	// Tạo uniform buffers để chứa dữ liệu camera (view, projection).
	CreateUniformBuffers();

	// --- 4. TẢI DỮ LIỆU SCENE ---
	// Khởi tạo các manager và tải các model, texture từ file.
	m_MeshManager = new MeshManager(m_VulkanContext->getVulkanHandles(), m_VulkanCommandManager);
	m_TextureManager = new TextureManager(m_VulkanContext->getVulkanHandles(), m_VulkanCommandManager, m_VulkanSampler->getSampler());
	m_MaterialManager = new MaterialManager(m_VulkanContext->getVulkanHandles(), m_VulkanCommandManager, m_TextureManager);
	m_LightManager = new LightManager(m_VulkanContext->getVulkanHandles(), m_VulkanCommandManager, m_Scene, m_VulkanSampler, MAX_FRAMES_IN_FLIGHT);

	// --- Khởi tạo Scene & Entities ---

	m_MainCamera = m_Scene->CreateEntity("Main Camera");
	auto& mainCameraData = m_Scene->GetRegistry().emplace<CameraComponent>(m_MainCamera);
	mainCameraData.AspectRatio = m_VulkanSwapchain->getHandles().swapChainExtent.width / (float)m_VulkanSwapchain->getHandles().swapChainExtent.height;
	
	auto& cameraTransform = m_Scene->GetRegistry().get<TransformComponent>(m_MainCamera);
	cameraTransform.SetPosition({ 0.0f, 3.0f, 5.0f });
	cameraTransform.SetRotation({ -11.0f, -90, 0 });
	
	// 1. Tải tài nguyên Model (chỉ tải một lần)
	m_AnimeGirlModel = new Model("Resources/AnimeGirl.assbin", m_MeshManager, m_MaterialManager);

	// 2. Tạo Entity: Girl 1
	m_Girl1 = m_Scene->CreateEntity("Girl1");
	// Gắn MeshComponent sử dụng model đã tải
	m_Scene->GetRegistry().emplace<MeshComponent>(m_Girl1, m_AnimeGirlModel, true);
	
	// Thiết lập vị trí ban đầu thông qua TransformComponent
	auto& girl1Transform = m_Scene->GetRegistry().get<TransformComponent>(m_Girl1);
	girl1Transform.SetPosition({ 1.0f, 0.0f, 0.0f });
	girl1Transform.SetRotation({ -90.0f, 0.0f, 0.0f });
	girl1Transform.SetScale({ 0.05f, 0.05f, 0.05f });

	// 3. Tạo Entity: Girl 2
	m_Girl2 = m_Scene->CreateEntity("Girl2");
	// Tái sử dụng cùng model cho entity thứ 2
	m_Scene->GetRegistry().emplace<MeshComponent>(m_Girl2, m_AnimeGirlModel, true);
	
	auto& girl2Transform = m_Scene->GetRegistry().get<TransformComponent>(m_Girl2);
	girl2Transform.SetPosition({ -1.0f, 0.0f, 0.0f });
	girl2Transform.SetRotation({ -90.0f, 0.0f, 0.0f });
	girl2Transform.SetScale({ 0.05f, 0.05f, 0.05f });


	// Hoàn tất việc tải texture: upload dữ liệu ảnh lên GPU và tạo descriptor set cho texture.
	m_MaterialManager->Finalize();
	m_TextureManager->FinalizeSetup();

	// --- 5. TẠO CÁC RENDER PASS ---
	// Khởi tạo các đối tượng cho từng bước trong chuỗi render (Geometry, Bright, Blur, Composite).
	CreateRenderPasses();

	// --- 6. HOÀN TẤT DESCRIPTORS VÀ ĐỒNG BỘ HÓA ---
	// Tổng hợp tất cả các descriptor từ các pass và tạo descriptor pool.
	m_VulkanDescriptorManager = new VulkanDescriptorManager(m_VulkanContext->getVulkanHandles());
	m_VulkanDescriptorManager->AddDescriptors(m_GeometryPass->GetHandles().descriptors);
	m_VulkanDescriptorManager->AddDescriptors(m_LightingPass->GetHandles().descriptors);
	m_VulkanDescriptorManager->AddDescriptors(m_BrightFilterPass->GetHandles().descriptors);
	m_VulkanDescriptorManager->AddDescriptors(m_BlurHPass->GetHandles().descriptors);
	m_VulkanDescriptorManager->AddDescriptors(m_BlurVPass->GetHandles().descriptors);
	m_VulkanDescriptorManager->AddDescriptors(m_CompositePass->GetHandles().descriptors);
	m_VulkanDescriptorManager->Finalize(); // Tạo pool và cấp phát các set.

	// Tạo các đối tượng đồng bộ (semaphores, fences) để điều phối vòng lặp render.
	m_VulkanSyncManager = new VulkanSyncManager(m_VulkanContext->getVulkanHandles(), MAX_FRAMES_IN_FLIGHT, m_VulkanSwapchain->getHandles().swapchainImageCount);

	// --- 7. TẢI DỮ LIỆU LÊN GPU ---
	// Sau khi tất cả các mesh đã được xử lý, tạo và tải dữ liệu vào vertex/index buffer trên GPU.
	m_MeshManager->CreateBuffers();

}

/**
 * @brief Destructor: Dọn dẹp tất cả tài nguyên đã cấp phát.
 * Phải đảm bảo đợi GPU hoàn thành mọi tác vụ trước khi giải phóng tài nguyên.
 * Thứ tự dọn dẹp rất quan trọng, thường là ngược lại với thứ tự tạo để tránh lỗi phụ thuộc.
 */
Application::~Application()
{
	// Đảm bảo GPU đã thực thi xong tất cả các lệnh trước khi bắt đầu hủy tài nguyên.
	vkDeviceWaitIdle(m_VulkanContext->getVulkanHandles().device);

	// 1. Giải phóng các Render Pass.
	delete(m_GeometryPass);
	delete(m_ShadowMapPass);
	delete(m_LightingPass);
	delete(m_BrightFilterPass);
	delete(m_BlurVPass);
	delete(m_BlurHPass);
	delete(m_CompositePass);

	// 2. Giải phóng các Manager.
	// DescriptorManager phải được hủy trước các tài nguyên mà nó quản lý (như uniform buffers, images).
	delete(m_VulkanDescriptorManager);
	delete(m_VulkanSyncManager);
	delete(m_VulkanCommandManager);
	delete(m_MeshManager);
	delete(m_TextureManager);
	delete(m_MaterialManager);
	delete(m_LightManager);

	// 3. Giải phóng Buffers (ví dụ: uniform buffers).
	for (auto& uniformBuffer : m_Geometry_UniformBuffers)
	{
		delete(uniformBuffer);
	}

	// 4. Giải phóng Images (các attachment của framebuffer).
	for (auto& image : m_LitSceneImages)
	{
		delete(image);
	}
	for (auto& image : m_TempBlurImages)
	{
		delete(image);
	}
	for (auto& image : m_BrightImages)
	{
		delete(image);
	}
	for (auto& image : m_Geometry_DepthStencilImage)
	{
		delete(image);
	}
	for (auto& image : m_Geometry_AlbedoImages)
	{
		delete(image);
	}
	for (auto& image : m_Geometry_NormalImages)
	{
		delete(image);
	}
	for (auto& image : m_Geometry_PositionImages)
	{
		delete(image);
	}
	delete(m_Composite_ColorImage);

	// 5. Giải phóng các đối tượng trong Scene.
	delete(m_AnimeGirlModel);

	// 6. Giải phóng các thành phần Vulkan cốt lõi.
	delete(m_VulkanSampler);
	delete(m_VulkanSwapchain);
	delete(m_VulkanContext); // Context phải được xóa gần cuối cùng vì nhiều đối tượng khác phụ thuộc vào nó.

	// 7. Giải phóng cửa sổ.
	delete(m_Window);
}

/**
 * @brief Vòng lặp chính của ứng dụng.
 * Liên tục xử lý sự kiện cửa sổ và gọi hàm DrawFrame cho đến khi cửa sổ nhận được yêu cầu đóng.
 */
void Application::Loop()
{
	while (!m_Window->windowShouldClose())
	{
		m_Window->windowPollEvents();
		DrawFrame();
	}

	// Đợi device rảnh rỗi trước khi thoát chương trình.
	vkDeviceWaitIdle(m_VulkanContext->getVulkanHandles().device);
}

// =================================================================================================
// SECTION 3: LỚP APPLICATION - LOGIC TRUNG TÂM (UPDATE & DRAW)
// =================================================================================================

/**
 * @brief Thực hiện tất cả các hoạt động cần thiết để vẽ một frame.
 * Đây là trái tim của vòng lặp render, điều phối việc đồng bộ CPU-GPU,
 * cập nhật dữ liệu, ghi command buffer và trình chiếu kết quả lên màn hình.
 */
void Application::DrawFrame()
{
	// --- 1. ĐỒNG BỘ CPU-GPU: ĐỢI FRAME TRƯỚC HOÀN THÀNH ---
	// Chờ fence của frame hiện tại, đảm bảo rằng command buffer từ lần lặp trước của frame này đã thực thi xong.
	vkWaitForFences(m_VulkanContext->getVulkanHandles().device, 1, &m_VulkanSyncManager->getCurrentFence(m_CurrentFrame), VK_TRUE, UINT64_MAX);

	// --- 2. LẤY ẢNH TIẾP THEO TỪ SWAPCHAIN ---
	// Yêu cầu một ảnh từ swapchain để chuẩn bị vẽ lên.
	// `imageIndex` là chỉ số của ảnh trong swapchain mà chúng ta sẽ render tới.
	// `imageAvailableSemaphore` sẽ được báo hiệu khi ảnh này sẵn sàng.
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_VulkanContext->getVulkanHandles().device, m_VulkanSwapchain->getHandles().swapchain, UINT64_MAX,
		m_VulkanSyncManager->getCurrentImageAvailableSemaphore(m_CurrentFrame),
		VK_NULL_HANDLE, &imageIndex);

	// Xử lý trường hợp swapchain không còn tương thích (ví dụ: cửa sổ bị resize).
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		// TODO: Implement lại swapchain.
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("LỖI: Không thể lấy ảnh từ swapchain!");
	}

	// --- 3. CHUẨN BỊ CHO LẦN SUBMIT MỚI ---
	// Reset fence về trạng thái "chưa báo hiệu" vì chúng ta sắp submit một command buffer mới và sẽ chờ nó.
	vkResetFences(m_VulkanContext->getVulkanHandles().device, 1, &m_VulkanSyncManager->getCurrentFence(m_CurrentFrame));

	// --- 4. CẬP NHẬT DỮ LIỆU ĐỘNG ---
	// Cập nhật dữ liệu sẽ thay đổi mỗi frame, ví dụ như ma trận camera, vị trí đối tượng.
	Update();

	// --- 5. GHI COMMAND BUFFER ---
	// Reset và ghi lại command buffer với các lệnh vẽ cho frame hiện tại.
	vkResetCommandBuffer(m_VulkanCommandManager->getHandles().commandBuffers[m_CurrentFrame], 0);
	RecordCommandBuffer(m_VulkanCommandManager->getHandles().commandBuffers[m_CurrentFrame], imageIndex);

	// --- 6. SUBMIT COMMAND BUFFER LÊN HÀNG ĐỢI ---
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_VulkanCommandManager->getHandles().commandBuffers[m_CurrentFrame];

	// Chỉ định semaphore để đợi: đợi `imageAvailableSemaphore` trước khi thực thi giai đoạn ghi màu.
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_VulkanSyncManager->getCurrentImageAvailableSemaphore(m_CurrentFrame);
	submitInfo.pWaitDstStageMask = &waitStage;

	// Chỉ định semaphore để báo hiệu: báo hiệu `renderFinishedSemaphore` khi command buffer thực thi xong.
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_VulkanSyncManager->getCurrentRenderFinishedSemaphore(imageIndex);

	// Submit command buffer lên graphics queue, và báo hiệu `inFlightFence` khi hoàn tất.
	VK_CHECK(vkQueueSubmit(m_VulkanContext->getVulkanHandles().graphicQueue, 1, &submitInfo, m_VulkanSyncManager->getCurrentFence(m_CurrentFrame)),
		"LỖI: Submit command buffer thất bại!");

	// --- 7. TRÌNH CHIẾU (PRESENT) ---
	// Đưa ảnh đã render xong ra màn hình.
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_VulkanSwapchain->getHandles().swapchain;
	presentInfo.pImageIndices = &imageIndex; // Chỉ định swapchain image nào sẽ được present.

	// Đợi `renderFinishedSemaphore` trước khi trình chiếu.
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_VulkanSyncManager->getCurrentRenderFinishedSemaphore(imageIndex);

	result = vkQueuePresentKHR(m_VulkanContext->getVulkanHandles().presentQueue, &presentInfo);

	// Xử lý lỗi nếu swapchain không còn hợp lệ.
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		// TODO: Implement lại swapchain.
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("LỖI: Trình chiếu ảnh swapchain thất bại!");
	}

	// --- 8. CHUYỂN SANG FRAME TIẾP THEO ---
	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

/**
 * @brief Cập nhật Uniform Buffer Object (UBO) với ma trận camera hiện tại.
 * Dữ liệu này được sử dụng trong Geometry Pass để định vị camera trong không gian 3D.
 */
void Application::Update_Geometry_Uniforms()
{
	auto view = m_Scene->GetRegistry().view<TransformComponent, CameraComponent>();
	view.each([&](auto e, const TransformComponent& transform, const CameraComponent& camera) 
		{
			if (camera.IsPrimary == false) return;

			m_Geometry_Ubo.view = TransformSystem::GetViewMatrix(transform);
			m_Geometry_Ubo.proj = camera.GetProjectionMatrix();
			m_Geometry_Ubo.viewPos = transform.GetPosition();

			m_Geometry_UniformBuffers[m_CurrentFrame]->UploadData(&m_Geometry_Ubo, sizeof(m_Geometry_Ubo), 0);
		});
}

/**
 * @brief Cập nhật transform (vị trí, xoay, tỷ lệ) của các đối tượng trong scene.
 * Hàm này dùng để tạo animation đơn giản cho các đối tượng.
 */
void Application::UpdateRenderObjectTransforms()
{	
	// Animation đơn giản dựa trên thời gian thực.
	static auto startTime = std::chrono::high_resolution_clock::now();
	static auto lastTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
	lastTime = currentTime;

	// Cập nhật logic xoay cho các entity
	auto& girl1Transform = m_Scene->GetRegistry().get<TransformComponent>(m_Girl1);
	auto& girl2Transform = m_Scene->GetRegistry().get<TransformComponent>(m_Girl2);

	girl1Transform.Rotate({ 0, deltaTime * MODEL_ROTATE_SPEED, 0 });
	girl2Transform.Rotate({ 0, -deltaTime * MODEL_ROTATE_SPEED, 0 });
}

// =================================================================================================
// SECTION 4: LỚP APPLICATION - GHI COMMAND BUFFER
// =================================================================================================

/**
 * @brief Ghi lại tất cả các lệnh render vào một command buffer.
 * Đây là chuỗi render (post-processing pipeline) cho hiệu ứng bloom:
 * 1. Geometry Pass: Render scene 3D vào một texture (m_SceneImages).
 * 2. Bright Pass: Lọc ra các vùng sáng từ scene texture (-> m_BrightImages).
 * 3. BlurH Pass: Làm mờ ngang vùng sáng (-> m_TempBlurImages).
 * 4. BlurV Pass: Làm mờ dọc kết quả (-> m_BrightImages, ghi đè).
 * 5. Composite Pass: Tổng hợp scene texture gốc và texture đã làm mờ (bloom)
 *    lên swapchain image để hiển thị.
 */
void Application::RecordCommandBuffer(const VkCommandBuffer& cmdBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo cmdBeginInfo{};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmdBeginInfo), "LỖI: Bắt đầu ghi command buffer thất bại!");

	// Thực thi tuần tự các render pass.
	m_GeometryPass->Execute(&cmdBuffer, imageIndex, m_CurrentFrame);
	m_ShadowMapPass->Execute(&cmdBuffer, imageIndex, m_CurrentFrame);
	m_LightingPass->Execute(&cmdBuffer, imageIndex, m_CurrentFrame);
	m_BrightFilterPass->Execute(&cmdBuffer, imageIndex, m_CurrentFrame);
	m_BlurHPass->Execute(&cmdBuffer, imageIndex, m_CurrentFrame);
	m_BlurVPass->Execute(&cmdBuffer, imageIndex, m_CurrentFrame);
	m_CompositePass->Execute(&cmdBuffer, imageIndex, m_CurrentFrame);

	VK_CHECK(vkEndCommandBuffer(cmdBuffer), "LỖI: Kết thúc ghi command buffer thất bại!");
}

   
// =================================================================================================
// SECTION 5: LỚP APPLICATION - CÁC HÀM HELPER KHỞI TẠO
// =================================================================================================

/**
 * @brief Tạo tất cả các VulkanImage sẽ được dùng làm attachment cho các bước render.
 * Các image này là các framebuffer trung gian cho chuỗi post-processing.
 */
/**
 * @brief Tạo tất cả các VulkanImage sẽ được dùng làm attachment cho các render pass.
 * Hàm này thiết lập các framebuffer trung gian cho G-Buffer, pass chính và các bước hậu xử lý.
 */
void Application::CreateFrameBufferImages()
{
	VkExtent2D swapchainExtent = m_VulkanSwapchain->getHandles().swapChainExtent;
	VkFormat swapchainFormat = m_VulkanSwapchain->getHandles().swapchainSupportDetails.chosenFormat.format;

	// =================================================================================================
	// I. TÀI NGUYÊN G-BUFFER (KHÔNG-MSAA)
	// =================================================================================================
	// Các attachment này được sử dụng trong GeometryPass để lưu trữ dữ liệu bề mặt.
	// Chúng không sử dụng MSAA để tiết kiệm bộ nhớ và vì việc resolve dữ liệu vị trí/pháp tuyến không có ý nghĩa.
	// Chúng cần cờ USAGE_SAMPLED_BIT để có thể được đọc trong Lighting Pass.

	// --- 1. G-Buffer: Albedo (Màu khuếch tán) ---
	// Lưu màu sắc cơ bản của vật thể, sử dụng định dạng của swapchain.
	VulkanImageCreateInfo gbufferAlbedoCI{};
	gbufferAlbedoCI.width = swapchainExtent.width;
	gbufferAlbedoCI.height = swapchainExtent.height;
	gbufferAlbedoCI.mipLevels = 1;
	gbufferAlbedoCI.samples = MSAA_SAMPLES;
	gbufferAlbedoCI.format = swapchainFormat;
	gbufferAlbedoCI.imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	gbufferAlbedoCI.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VulkanImageViewCreateInfo gbufferAlbedoCVI{};
	gbufferAlbedoCVI.format = swapchainFormat;
	gbufferAlbedoCVI.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	gbufferAlbedoCVI.mipLevels = 1;

	// --- 2. G-Buffer: Normal & Position (Pháp tuyến & Vị trí) ---
	// Sử dụng định dạng float 16-bit (R16G16B16A16) để có độ chính xác cao cho dữ liệu world-space.
	VulkanImageCreateInfo gbufferHiPrecisionCI{};
	gbufferHiPrecisionCI.width = swapchainExtent.width;
	gbufferHiPrecisionCI.height = swapchainExtent.height;
	gbufferHiPrecisionCI.mipLevels = 1;
	gbufferHiPrecisionCI.samples = MSAA_SAMPLES;
	gbufferHiPrecisionCI.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	gbufferHiPrecisionCI.imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	gbufferHiPrecisionCI.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VulkanImageViewCreateInfo gbufferHiPrecisionCVI{};
	gbufferHiPrecisionCVI.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	gbufferHiPrecisionCVI.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	gbufferHiPrecisionCVI.mipLevels = 1;

	// --- 3. G-Buffer: Depth/Stencil (Độ sâu) ---
	// Depth buffer không-MSAA dành riêng cho Geometry Pass.
	VulkanImageCreateInfo gbufferDepthCI{};
	gbufferDepthCI.width = swapchainExtent.width;
	gbufferDepthCI.height = swapchainExtent.height;
	gbufferDepthCI.mipLevels = 1;
	gbufferDepthCI.samples = MSAA_SAMPLES;
	gbufferDepthCI.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
	gbufferDepthCI.imageUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	gbufferDepthCI.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VulkanImageViewCreateInfo gbufferDepthCVI{};
	gbufferDepthCVI.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
	gbufferDepthCVI.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	gbufferDepthCVI.mipLevels = 1;

	// Cấp phát bộ nhớ cho các vector chứa G-Buffer images
	m_Geometry_AlbedoImages.resize(MAX_FRAMES_IN_FLIGHT);
	m_Geometry_NormalImages.resize(MAX_FRAMES_IN_FLIGHT);
	m_Geometry_PositionImages.resize(MAX_FRAMES_IN_FLIGHT);
	m_Geometry_DepthStencilImage.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_Geometry_AlbedoImages[i] = new VulkanImage(m_VulkanContext->getVulkanHandles(), gbufferAlbedoCI, gbufferAlbedoCVI);
		m_Geometry_NormalImages[i] = new VulkanImage(m_VulkanContext->getVulkanHandles(), gbufferHiPrecisionCI, gbufferHiPrecisionCVI);
		m_Geometry_PositionImages[i] = new VulkanImage(m_VulkanContext->getVulkanHandles(), gbufferHiPrecisionCI, gbufferHiPrecisionCVI);
		m_Geometry_DepthStencilImage[i] = new VulkanImage(m_VulkanContext->getVulkanHandles(), gbufferDepthCI, gbufferDepthCVI);
	}

	// =================================================================================================
	// II. TÀI NGUYÊN CHO PASS CHÍNH (CÓ-MSAA)
	// =================================================================================================
	// Các attachment này sẽ được sử dụng trong Lighting Pass (hoặc Composite Pass hiện tại)
	// để thực hiện tính toán ánh sáng và khử răng cưa.

	// --- 4. Main Pass: Color (MSAA) ---
	VulkanImageCreateInfo mainColorMSAA_CI{};
	mainColorMSAA_CI.width = swapchainExtent.width;
	mainColorMSAA_CI.height = swapchainExtent.height;
	mainColorMSAA_CI.mipLevels = 1;
	mainColorMSAA_CI.samples = MSAA_SAMPLES;
	mainColorMSAA_CI.format = swapchainFormat;
	mainColorMSAA_CI.imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	mainColorMSAA_CI.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VulkanImageViewCreateInfo mainColorMSAA_CVI{};
	mainColorMSAA_CVI.format = swapchainFormat;
	mainColorMSAA_CVI.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	mainColorMSAA_CVI.mipLevels = 1;

	m_Composite_ColorImage = new VulkanImage(m_VulkanContext->getVulkanHandles(), mainColorMSAA_CI, mainColorMSAA_CVI);

	// --- 5. Main Pass: Depth/Stencil (MSAA) ---
	VulkanImageCreateInfo mainDepthMSAA_CI{};
	mainDepthMSAA_CI.width = swapchainExtent.width;
	mainDepthMSAA_CI.height = swapchainExtent.height;
	mainDepthMSAA_CI.mipLevels = 1;
	mainDepthMSAA_CI.samples = MSAA_SAMPLES;
	mainDepthMSAA_CI.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
	mainDepthMSAA_CI.imageUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	mainDepthMSAA_CI.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VulkanImageViewCreateInfo mainDepthMSAA_CVI{};
	mainDepthMSAA_CVI.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
	mainDepthMSAA_CVI.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	mainDepthMSAA_CVI.mipLevels = 1;

	// =================================================================================================
	// III. TÀI NGUYÊN HẬU XỬ LÝ (POST-PROCESSING, KHÔNG-MSAA)
	// =================================================================================================
	
	// --- 6. Post-Processing: Ảnh trung gian (Non-MSAA) ---
	// Dùng cho hiệu ứng bloom (Bright Filter, Blur).
	VulkanImageCreateInfo postProcessingCI{};
	postProcessingCI.width = swapchainExtent.width;
	postProcessingCI.height = swapchainExtent.height;
	postProcessingCI.mipLevels = 1;
	postProcessingCI.samples = VK_SAMPLE_COUNT_1_BIT;
	postProcessingCI.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	postProcessingCI.imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	postProcessingCI.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VulkanImageViewCreateInfo postProcessingCVI{};
	postProcessingCVI.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	postProcessingCVI.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	postProcessingCVI.mipLevels = 1;

	m_LitSceneImages.resize(MAX_FRAMES_IN_FLIGHT);
	m_BrightImages.resize(MAX_FRAMES_IN_FLIGHT);
	m_TempBlurImages.resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_LitSceneImages[i] = new VulkanImage(m_VulkanContext->getVulkanHandles(), postProcessingCI, postProcessingCVI);
		m_BrightImages[i] = new VulkanImage(m_VulkanContext->getVulkanHandles(), postProcessingCI, postProcessingCVI);
		m_TempBlurImages[i] = new VulkanImage(m_VulkanContext->getVulkanHandles(), postProcessingCI, postProcessingCVI);
	} 
}
 

void Application::CreateSceneLights()
{
	// --- KEY LIGHT: Directional Light (Trắng) ---
	glm::vec3 dirLightMain = glm::normalize(glm::vec3(0.4f, .2f, -0.6f));

	Light light1 = Light::CreateDirectional(
		dirLightMain,
		glm::vec3(1.0f, 0.95f, 0.9f),   // Trắng hơi ấm → rất đẹp cho da/model
		4.0f,                           // Cường độ vừa phải
		true                            // Có đổ bóng
	);

	// --- FILL/RIM LIGHT: Spot Light (Xanh dương nhạt) ---
	glm::vec3 targetPosition = glm::vec3(0, 2, 0);
	glm::vec3 posSpot = glm::vec3(2.0f, 3.0f, 2.0f);
	glm::vec3 dirSpot = glm::normalize(targetPosition - posSpot);

	Light light2 = Light::CreateSpot(
		dirSpot,
		glm::vec3(0.25f, 0.35f, 0.45f),  // Xanh dương nhẹ → tạo chiều sâu
		100.0f,                            // Cường độ nhỏ hơn đèn chính
		100.0f,                           // Range vừa đủ
		20.0f,                           // Inner cutoff mềm
		40.0f,                           // Outer cutoff mượt
		true
	);

	m_Light1 = m_Scene->CreateEntity("Light1");
	m_Light2 = m_Scene->CreateEntity("Light2");

	m_Scene->GetRegistry().emplace<LightComponent>(m_Light1, light1, true);

	m_Scene->GetRegistry().emplace<LightComponent>(m_Light2, light2, true);
	auto& light2Transform = m_Scene->GetRegistry().get<TransformComponent>(m_Light2);
	light2Transform.SetPosition(posSpot);
}
/**
 * @brief Tạo các đối tượng render pass.
 * Mỗi pass đóng gói một pipeline và logic để thực thi một bước render cụ thể.
 */
void Application::CreateRenderPasses()
{
	// --- 1. Geometry Pass ---
	// Vẽ scene 3D vào một framebuffer trung gian (RTT).
	GeometryPassCreateInfo geometryInfo{};
	geometryInfo.albedoImages = &m_Geometry_AlbedoImages;
	geometryInfo.normalImages = &m_Geometry_NormalImages;
	geometryInfo.positionImages = &m_Geometry_PositionImages;
	geometryInfo.textureManager = m_TextureManager;
	geometryInfo.meshManager = m_MeshManager;
	geometryInfo.materialManager = m_MaterialManager;
	geometryInfo.depthStencilImages = &m_Geometry_DepthStencilImage;
	geometryInfo.BackgroundColor = BACKGROUND_COLOR;
	geometryInfo.vulkanSwapchainHandles = &m_VulkanSwapchain->getHandles();
	geometryInfo.scene = m_Scene;
	geometryInfo.vulkanHandles = &m_VulkanContext->getVulkanHandles();
	geometryInfo.MSAA_SAMPLES = MSAA_SAMPLES;
	geometryInfo.fragShaderFilePath = "Shaders/Geometry_Shader.frag.spv";
	geometryInfo.vertShaderFilePath = "Shaders/Geometry_Shader.vert.spv";
	geometryInfo.uniformBuffers = &m_Geometry_UniformBuffers;
	m_GeometryPass = new GeometryPass(geometryInfo);

	// Shadow Map Pass
	ShadowMapPassCreateInfo shadowInfo{};
	shadowInfo.BackgroundColor = BACKGROUND_COLOR;
	shadowInfo.fragShaderFilePath = "Shaders/ShadowMap_Shader.frag.spv";
	shadowInfo.vertShaderFilePath = "Shaders/ShadowMap_Shader.vert.spv";
	shadowInfo.lightManager = m_LightManager;
	shadowInfo.MAX_FRAMES_IN_FLIGHT = MAX_FRAMES_IN_FLIGHT;
	shadowInfo.meshManager = m_MeshManager;
	shadowInfo.MSAA_SAMPLES = VK_SAMPLE_COUNT_1_BIT;
	shadowInfo.scene = m_Scene;
	shadowInfo.vulkanHandles = &m_VulkanContext->getVulkanHandles();
	shadowInfo.vulkanSwapchainHandles = &m_VulkanSwapchain->getHandles();

	m_ShadowMapPass = new ShadowMapPass(shadowInfo);
	// --- 2. Lighting Pass ---
	// Thực hiện tính toán ánh sáng bằng cách sử dụng G-Buffer.
	LightingPassCreateInfo lightingInfo{};
	lightingInfo.vulkanHandles = &m_VulkanContext->getVulkanHandles();
	lightingInfo.vulkanSwapchainHandles = &m_VulkanSwapchain->getHandles();
	lightingInfo.MSAA_SAMPLES = MSAA_SAMPLES;
	lightingInfo.MAX_FRAMES_IN_FLIGHT = MAX_FRAMES_IN_FLIGHT;
	lightingInfo.fragShaderFilePath = "Shaders/Lighting_Shader.frag.spv";
	lightingInfo.vertShaderFilePath = "Shaders/PostProcess_Shader.vert.spv";
	lightingInfo.BackgroundColor = BACKGROUND_COLOR;
	lightingInfo.outputImages = &m_LitSceneImages;
	lightingInfo.gAlbedoTextures = &m_Geometry_AlbedoImages;
	lightingInfo.gNormalTextures = &m_Geometry_NormalImages; 
	lightingInfo.gPositionTextures = &m_Geometry_PositionImages;
	lightingInfo.sceneLightDescriptors = &m_LightManager->GetDescriptors();
	lightingInfo.uniformBuffers = &m_Geometry_UniformBuffers; // Pass camera UBO
	lightingInfo.vulkanSampler = m_VulkanSampler;
	m_LightingPass = new LightingPass(lightingInfo);

	// --- 3. Bright Filter Pass ---
	// Lọc ra các vùng sáng từ ảnh đã được chiếu sáng để chuẩn bị cho hiệu ứng bloom.
	BrightFilterPassCreateInfo brightFilterInfo{};
	brightFilterInfo.vulkanHandles = &m_VulkanContext->getVulkanHandles();
	brightFilterInfo.vulkanSwapchainHandles = &m_VulkanSwapchain->getHandles();
	brightFilterInfo.MSAA_SAMPLES = VK_SAMPLE_COUNT_1_BIT; // Post-processing không cần MSAA.
	brightFilterInfo.MAX_FRAMES_IN_FLIGHT = MAX_FRAMES_IN_FLIGHT;
	brightFilterInfo.fragShaderFilePath = "Shaders/Bright_Shader.frag.spv";
	brightFilterInfo.vertShaderFilePath = "Shaders/PostProcess_Shader.vert.spv"; // Dùng chung vertex shader vẽ quad.
	brightFilterInfo.BackgroundColor = BACKGROUND_COLOR;
	brightFilterInfo.outputImage = &m_BrightImages;
	brightFilterInfo.inputTextures = &m_LitSceneImages; // Input là ảnh đã được chiếu sáng.
	brightFilterInfo.vulkanSampler = m_VulkanSampler;
	m_BrightFilterPass = new BrightFilterPass(brightFilterInfo);

	// --- 4. Horizontal Blur Pass ---
	// Làm mờ ảnh chứa các vùng sáng theo chiều ngang.
	BlurPassCreateInfo blurHInfo{};
	blurHInfo.vulkanHandles = &m_VulkanContext->getVulkanHandles();
	blurHInfo.vulkanSwapchainHandles = &m_VulkanSwapchain->getHandles();
	blurHInfo.MSAA_SAMPLES = VK_SAMPLE_COUNT_1_BIT;
	blurHInfo.fragShaderFilePath = "Shaders/BlurH_Shader.frag.spv";
	blurHInfo.vertShaderFilePath = "Shaders/PostProcess_Shader.vert.spv";
	blurHInfo.BackgroundColor = BACKGROUND_COLOR;
	blurHInfo.outputImages = &m_TempBlurImages; // Ghi kết quả vào ảnh tạm.
	blurHInfo.inputTextures = &m_BrightImages; // Input là ảnh các vùng sáng.
	blurHInfo.vulkanSampler = m_VulkanSampler;
	m_BlurHPass = new BlurPass(blurHInfo);

	// --- 5. Vertical Blur Pass ---
	// Làm mờ ảnh đã được làm mờ ngang theo chiều dọc.
	BlurPassCreateInfo blurVInfo{};
	blurVInfo.vulkanHandles = &m_VulkanContext->getVulkanHandles();
	blurVInfo.vulkanSwapchainHandles = &m_VulkanSwapchain->getHandles();
	blurVInfo.MSAA_SAMPLES = VK_SAMPLE_COUNT_1_BIT;
	blurVInfo.fragShaderFilePath = "Shaders/BlurV_Shader.frag.spv";
	blurVInfo.vertShaderFilePath = "Shaders/PostProcess_Shader.vert.spv";
	blurVInfo.BackgroundColor = BACKGROUND_COLOR;
	blurVInfo.outputImages = &m_BrightImages; // Ghi đè kết quả vào ảnh chứa vùng sáng.
	blurVInfo.inputTextures = &m_TempBlurImages; // Input là ảnh đã blur ngang.
	blurVInfo.vulkanSampler = m_VulkanSampler;
	m_BlurVPass = new BlurPass(blurVInfo);

	// --- 6. Composite Pass ---
	// Tổng hợp ảnh scene đã chiếu sáng và ảnh bloom cuối cùng.
	CompositePassCreateInfo compositeInfo{};
	compositeInfo.vulkanHandles = &m_VulkanContext->getVulkanHandles();
	compositeInfo.vulkanSwapchainHandles = &m_VulkanSwapchain->getHandles();
	compositeInfo.MSAA_SAMPLES = MSAA_SAMPLES;
	compositeInfo.fragShaderFilePath = "Shaders/Composite_Shader.frag.spv";
	compositeInfo.vertShaderFilePath = "Shaders/PostProcess_Shader.vert.spv";
	compositeInfo.BackgroundColor = BACKGROUND_COLOR;
	compositeInfo.mainColorImage = m_Composite_ColorImage;
	compositeInfo.inputTextures0 = &m_LitSceneImages;       // Input 1: Ảnh scene đã chiếu sáng.
	compositeInfo.inputTextures1 = &m_BrightImages;        // Input 2: Ảnh bloom đã xử lý.
	compositeInfo.vulkanSampler = m_VulkanSampler;
	m_CompositePass = new CompositePass(compositeInfo);
}

/**
 * @brief Tạo uniform buffers (UBO) cho Geometry pass.
 * Các buffer này chứa ma trận View-Projection (camera).
 * Tạo một UBO cho mỗi frame-in-flight để tránh xung đột dữ liệu khi CPU cập nhật và GPU đang đọc.
 */
void Application::CreateUniformBuffers()
{
	m_Geometry_UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.queueFamilyIndexCount = 1;
	bufferInfo.pQueueFamilyIndices = &m_VulkanContext->getVulkanHandles().queueFamilyIndices.GraphicQueueIndex;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.size = sizeof(UniformBufferObject);
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		// Tạo buffer với VMA_MEMORY_USAGE_CPU_TO_GPU để CPU có thể ghi dữ liệu trực tiếp.
		m_Geometry_UniformBuffers[i] = new VulkanBuffer(m_VulkanContext->getVulkanHandles(), m_VulkanCommandManager, bufferInfo, VMA_MEMORY_USAGE_CPU_TO_GPU);
	}
}

void Application::Update()
{
	Update_Geometry_Uniforms();
	UpdateRenderObjectTransforms();

	TransformSystem::UpdateTransformMatrix(m_Scene);
}

