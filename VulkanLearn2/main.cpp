#include "pch.h"
#include "main.h"

#include "Core/Window.h"
#include "Core/VulkanSwapchain.h"
#include "Core/VulkanImage.h"
#include "Core/VulkanRenderPass.h"
#include "Core/VulkanCommandManager.h"
#include "Core/VulkanPipeline.h"
#include "Core/VulkanSyncManager.h"
#include "Core/VulkanBuffer.h"
#include "Core/VulkanDescriptorManager.h"
#include "Core/VulkanSampler.h"
#include "Core/VulkanDescriptor.h"
#include "Core/VulkanFrameBuffer.h"

#include "Utils/DebugTimer.h"
#include <Scene/RenderObject.h>
#include "Scene/MeshManager.h"
#include "Scene/Model.h"
#include "Scene/TextureManager.h"

int main()
{
	Application app;
	app.Loop();
}

// Constructor: Khởi tạo toàn bộ ứng dụng Vulkan
Application::Application()
{
	// 1. Tạo cửa sổ
	m_Window = new Window(WINDOW_WIDTH, WINDOW_HEIGHT, "ZOLCOL VULKAN");

	// 2. Khởi tạo các thành phần Vulkan cốt lõi
	m_VulkanContext = new VulkanContext(m_Window->getGLFWWindow(), m_Window->getInstanceExtensionsRequired());
	m_VulkanSwapchain = new VulkanSwapchain(m_VulkanContext->getVulkanHandles(), m_Window->getGLFWWindow());
	m_VulkanRenderPass = new VulkanRenderPass(m_VulkanContext->getVulkanHandles(), m_VulkanSwapchain->getHandles(), MSAA_SAMPLES);
	m_VulkanCommandManager = new VulkanCommandManager(m_VulkanContext->getVulkanHandles(), MAX_FRAMES_IN_FLIGHT);
	m_VulkanSampler = new VulkanSampler(m_VulkanContext->getVulkanHandles());
	
	CreateFrameBuffers();

	// 3. Tạo uniform buffers, một cái cho mỗi frame-in-flight
	CreateUniformBuffers();
	CreateMainDescriptors();
	CreateBrightDescriptors();

	// 4. Tạo các manager cho scene và tải object
	m_MeshManager = new MeshManager(m_VulkanContext->getVulkanHandles(), m_VulkanCommandManager);
	m_TextureManager = new TextureManager(m_VulkanContext->getVulkanHandles(), m_VulkanCommandManager, m_VulkanSampler->getSampler());

	m_BunnyGirl = new RenderObject("Resources/bunnyGirl.assbin", m_MeshManager, m_TextureManager);
	m_Swimsuit = new RenderObject("Resources/swimSuit.assbin", m_MeshManager, m_TextureManager);
	m_BunnyGirl->SetRotation({ -90, 0, 0 });
	
	m_RenderObjects.push_back(m_BunnyGirl);
	m_RenderObjects.push_back(m_Swimsuit);

	// Hoàn tất việc tải texture và lấy descriptor của chúng
	m_TextureManager->FinalizeSetup();
	m_RTTPipelineDescriptors.push_back(m_TextureManager->getDescriptor());

	// 5. Tạo descriptor manager và graphics pipeline
	m_allDescriptors.insert(m_allDescriptors.end(), m_RTTPipelineDescriptors.begin(), m_RTTPipelineDescriptors.end());
	m_allDescriptors.insert(m_allDescriptors.end(), m_MainPipelineDescriptors.begin(), m_MainPipelineDescriptors.end());
	m_allDescriptors.insert(m_allDescriptors.end(), m_BrightTextureDescriptors.begin(), m_BrightTextureDescriptors.end());
	
	m_VulkanDescriptorManager = new VulkanDescriptorManager(m_VulkanContext->getVulkanHandles(), m_allDescriptors);

	CreatePipelines();

	// 6. Tạo các đối tượng đồng bộ (semaphores, fences)
	m_VulkanSyncManager = new VulkanSyncManager(m_VulkanContext->getVulkanHandles(), MAX_FRAMES_IN_FLIGHT, m_VulkanSwapchain->getHandles().swapchainImageCount);

	// 7. Hoàn tất việc tạo buffer cho mesh
	m_MeshManager->CreateBuffers();
}

// Destructor: Dọn dẹp tất cả tài nguyên đã cấp phát
Application::~Application()
{
	// Đợi GPU hoàn thành mọi tác vụ trước khi dọn dẹp
	vkDeviceWaitIdle(m_VulkanContext->getVulkanHandles().device);

	// Thứ tự xóa rất quan trọng. Thường là ngược lại với thứ tự tạo và theo mối quan hệ phụ thuộc.

	// 1. Pipelines
	delete(m_BrightVulkanPipeline);
	delete(m_MainVulkanPipeline);
	delete(m_RTTVulkanPipeline);

	// 2. Managers
	delete(m_VulkanDescriptorManager);
	delete(m_VulkanSyncManager);
	delete(m_VulkanCommandManager);
	delete(m_MeshManager);
	delete(m_TextureManager);

	// 3. Buffers
	for (auto& uniformBuffer : m_RTT_UniformBuffers)
	{
		delete(uniformBuffer);
	}

	// 4. Framebuffers
	for (auto& frameBuffer : m_Bright_FrameBuffers)
	{
		delete(frameBuffer);
	}
	for (auto& frameBuffer : m_Main_FrameBuffers)
	{
		delete(frameBuffer);
	}
	for (auto& frameBuffer : m_RTT_FrameBuffers)
	{
		delete(frameBuffer);
	}

	// 5. Images
	for (auto& image : m_BrightImages)
	{
		delete(image);
	}
	for (auto& image : m_SceneImages)
	{
		delete(image);
	}
	delete(m_Main_DepthStencilImage);
	delete(m_Main_ColorImage);
	delete(m_RTT_DepthStencilImage);
	delete(m_RTT_ColorImage);

	// 6. Scene Objects
	for (auto& renderObject : m_RenderObjects)
	{
		delete(renderObject);
	}

	// 7. Core Vulkan Objects
	delete(m_VulkanSampler);
	delete(m_VulkanRenderPass);
	delete(m_VulkanSwapchain);
	delete(m_VulkanContext);

	// 8. Window
	delete(m_Window);
}

void Application::CreateFrameBufferImages()
{
	// Image cho RTT FrameBuffer
		// Color Image
	VulkanImageCreateInfo RTT_ColorImageInfo{};
	RTT_ColorImageInfo.width = m_VulkanSwapchain->getHandles().swapChainExtent.width;
	RTT_ColorImageInfo.height = m_VulkanSwapchain->getHandles().swapChainExtent.height;
	RTT_ColorImageInfo.mipLevels = 1;
	RTT_ColorImageInfo.samples = MSAA_SAMPLES;
	RTT_ColorImageInfo.format = m_VulkanSwapchain->getHandles().swapchainSupportDetails.chosenFormat.format;
	RTT_ColorImageInfo.imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	RTT_ColorImageInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VulkanImageViewCreateInfo RTT_ColorImageViewInfo{};
	RTT_ColorImageViewInfo.format = m_VulkanSwapchain->getHandles().swapchainSupportDetails.chosenFormat.format;
	RTT_ColorImageViewInfo.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	RTT_ColorImageViewInfo.mipLevels = 1;

	m_RTT_ColorImage = new VulkanImage(m_VulkanContext->getVulkanHandles(), RTT_ColorImageInfo, RTT_ColorImageViewInfo);

		// Depth Stencil Image
	VulkanImageCreateInfo RTT_DepthImageInfo{};
	RTT_DepthImageInfo.width = m_VulkanSwapchain->getHandles().swapChainExtent.width;
	RTT_DepthImageInfo.height = m_VulkanSwapchain->getHandles().swapChainExtent.height;
	RTT_DepthImageInfo.mipLevels = 1;
	RTT_DepthImageInfo.samples = MSAA_SAMPLES;
	RTT_DepthImageInfo.format = VK_FORMAT_D32_SFLOAT_S8_UINT; // Format phổ biến cho depth/stencil.
	RTT_DepthImageInfo.imageUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	RTT_DepthImageInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VulkanImageViewCreateInfo RTT_DepthImageViewInfo{};
	RTT_DepthImageViewInfo.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
	RTT_DepthImageViewInfo.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	RTT_DepthImageViewInfo.mipLevels = 1;

	m_RTT_DepthStencilImage = new VulkanImage(m_VulkanContext->getVulkanHandles(), RTT_DepthImageInfo, RTT_DepthImageViewInfo);
	
		// m_SceneImages
	m_SceneImages.resize(MAX_FRAMES_IN_FLIGHT);
	VulkanImageCreateInfo SceneImageInfo{};
	SceneImageInfo.width = m_VulkanSwapchain->getHandles().swapChainExtent.width;
	SceneImageInfo.height = m_VulkanSwapchain->getHandles().swapChainExtent.height;
	SceneImageInfo.mipLevels = 1;
	SceneImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	SceneImageInfo.format = m_VulkanSwapchain->getHandles().swapchainSupportDetails.chosenFormat.format;
	SceneImageInfo.imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	SceneImageInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VulkanImageViewCreateInfo SceneImageViewInfo{};
	SceneImageViewInfo.format = m_VulkanSwapchain->getHandles().swapchainSupportDetails.chosenFormat.format;
	SceneImageViewInfo.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	SceneImageViewInfo.mipLevels = 1;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_SceneImages[i] = new VulkanImage(m_VulkanContext->getVulkanHandles(), SceneImageInfo, SceneImageViewInfo);
	}

	// Image Cho Main Frame Buffer
	m_Main_ColorImage = new VulkanImage(m_VulkanContext->getVulkanHandles(), RTT_ColorImageInfo, RTT_ColorImageViewInfo);
	m_Main_DepthStencilImage = new VulkanImage(m_VulkanContext->getVulkanHandles(), RTT_DepthImageInfo, RTT_DepthImageViewInfo);

	// Image Cho Bright Frame Buffer
	m_BrightImages.resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_BrightImages[i] = new VulkanImage(m_VulkanContext->getVulkanHandles(), SceneImageInfo, SceneImageViewInfo);
	}
}

void Application::CreateFrameBuffers()
{
	// Tạo image cho frame Buffer
	CreateFrameBufferImages();

	// --- Khởi tạo RTT Frame Buffers ---
	m_RTT_FrameBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkImageView RTT_ImageViews[] = { 
			m_RTT_ColorImage->GetHandles().imageView, 
			m_RTT_DepthStencilImage->GetHandles().imageView, 
			m_SceneImages[i]->GetHandles().imageView 
		};

		FrameBufferCreateInfo RTT_FrameBufferInfo{};
		RTT_FrameBufferInfo.frameWidth = m_VulkanSwapchain->getHandles().swapChainExtent.width;
		RTT_FrameBufferInfo.frameHeigth = m_VulkanSwapchain->getHandles().swapChainExtent.height;
		RTT_FrameBufferInfo.frameLayer = 1;
		RTT_FrameBufferInfo.imageCount = 3;
		RTT_FrameBufferInfo.pVkImageView = RTT_ImageViews;

		m_RTT_FrameBuffers[i] = new VulkanFrameBuffer(m_VulkanContext->getVulkanHandles(), m_VulkanRenderPass->getHandles().rttRenderPass, RTT_FrameBufferInfo);
	}
	
	// --- Khởi tạo MAIN Frame Buffers ---
	m_Main_FrameBuffers.resize(m_VulkanSwapchain->getHandles().swapchainImageCount);
	for (int i = 0; i < m_VulkanSwapchain->getHandles().swapchainImageCount; i++)
	{
		VkImageView Main_ImageViews[] = {
			m_Main_ColorImage->GetHandles().imageView,
			m_Main_DepthStencilImage->GetHandles().imageView,
			m_VulkanSwapchain->getHandles().swapchainImageViews[i]
		};

		FrameBufferCreateInfo Main_FrameBufferInfo{};
		Main_FrameBufferInfo.frameWidth = m_VulkanSwapchain->getHandles().swapChainExtent.width;
		Main_FrameBufferInfo.frameHeigth = m_VulkanSwapchain->getHandles().swapChainExtent.height;
		Main_FrameBufferInfo.frameLayer = 1;
		Main_FrameBufferInfo.imageCount = 3;
		Main_FrameBufferInfo.pVkImageView = Main_ImageViews;

		m_Main_FrameBuffers[i] = new VulkanFrameBuffer(m_VulkanContext->getVulkanHandles(), m_VulkanRenderPass->getHandles().mainRenderPass, Main_FrameBufferInfo);
	}

	// --- Khởi tạo Bright Frame Buffers ---
	m_Bright_FrameBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkImageView Bright_ImageViews[] = {
			m_BrightImages[i]->GetHandles().imageView
		};

		FrameBufferCreateInfo Bright_FrameBufferInfo{};
		Bright_FrameBufferInfo.frameWidth = m_VulkanSwapchain->getHandles().swapChainExtent.width;
		Bright_FrameBufferInfo.frameHeigth = m_VulkanSwapchain->getHandles().swapChainExtent.height;
		Bright_FrameBufferInfo.frameLayer = 1;
		Bright_FrameBufferInfo.imageCount = 1;
		Bright_FrameBufferInfo.pVkImageView = Bright_ImageViews;

		m_Bright_FrameBuffers[i] = new VulkanFrameBuffer(m_VulkanContext->getVulkanHandles(), m_VulkanRenderPass->getHandles().brightRenderPass, Bright_FrameBufferInfo);
	}
}

// Tạo uniform buffers cho mỗi frame-in-flight để lưu trữ ma trận camera.
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
		// Tạo buffer
		m_RTT_UniformBuffers[i] = new VulkanBuffer(m_VulkanContext->getVulkanHandles(), m_VulkanCommandManager, bufferInfo, VMA_MEMORY_USAGE_CPU_TO_GPU);

		// Tạo descriptor cho buffer
		BindingElementInfo uniformElementInfo;
		uniformElementInfo.binding = 0;
		uniformElementInfo.pImmutableSamplers = nullptr;
		uniformElementInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformElementInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
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

		std::vector<BindingElementInfo> uniformBindings{ uniformElementInfo };
		m_RTT_UniformDescriptors[i] = new VulkanDescriptor(m_VulkanContext->getVulkanHandles(), uniformBindings, 1);
		
		// Thêm descriptor vào danh sách tất cả descriptor để tạo pipeline
		m_RTTPipelineDescriptors.push_back(m_RTT_UniformDescriptors[i]);
	}
}

void Application::CreateMainDescriptors()
{
	m_MainPipelineDescriptors.resize(MAX_FRAMES_IN_FLIGHT);
	m_MainTextureDescriptors.resize(MAX_FRAMES_IN_FLIGHT);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
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
		std::vector<VkDescriptorImageInfo> imageInfos{ imageInfo };
		imageUpdateInfo.imageInfos = imageInfos;

		bindingElement.imageDescriptorUpdateInfoCount = 1;
		bindingElement.pImageDescriptorUpdates = &imageUpdateInfo;

		std::vector<BindingElementInfo> bindingElements = { bindingElement };

		m_MainTextureDescriptors[i] = new VulkanDescriptor(m_VulkanContext->getVulkanHandles(), bindingElements, 0);
		m_MainPipelineDescriptors[i] = m_MainTextureDescriptors[i];
	}
}

void Application::CreateBrightDescriptors()
{
	m_BrightPipelineDescriptors.resize(MAX_FRAMES_IN_FLIGHT);
	m_BrightTextureDescriptors.resize(MAX_FRAMES_IN_FLIGHT);

	

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		// Scene Texture Binding
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

		m_BrightTextureDescriptors[i] = new VulkanDescriptor(m_VulkanContext->getVulkanHandles(), bindingElements, 0);
		m_BrightPipelineDescriptors[i] = m_BrightTextureDescriptors[i];
	}
}

void Application::CreatePipelines()
{
	// Create RTT PIPELINE
	VulkanPipelineCreateInfo rttPipelineInfo{};
	rttPipelineInfo.descriptors = &m_RTTPipelineDescriptors;
	rttPipelineInfo.fragmentShaderFilePath = "Shaders/RTT_Shader.frag.spv";
	rttPipelineInfo.vertexShaderFilePath = "Shaders/RTT_Shader.vert.spv";
	rttPipelineInfo.msaaSamples = MSAA_SAMPLES;
	rttPipelineInfo.renderPass = &m_VulkanRenderPass->getHandles().rttRenderPass;
	rttPipelineInfo.swapchainHandles = &m_VulkanSwapchain->getHandles();
	rttPipelineInfo.vulkanHandles = &m_VulkanContext->getVulkanHandles();

	m_RTTVulkanPipeline = new VulkanPipeline(&rttPipelineInfo);
	
	// CREATE MAIN PIPELINE
	VulkanPipelineCreateInfo mainPipelineInfo = rttPipelineInfo;
	mainPipelineInfo.fragmentShaderFilePath = "Shaders/Main_Shader.frag.spv";
	mainPipelineInfo.vertexShaderFilePath = "Shaders/Main_Shader.vert.spv";
	mainPipelineInfo.renderPass = &m_VulkanRenderPass->getHandles().mainRenderPass;
	mainPipelineInfo.descriptors = &m_MainPipelineDescriptors;
	mainPipelineInfo.useVertexInput = false;

	m_MainVulkanPipeline = new VulkanPipeline(&mainPipelineInfo);

	// CREATE BRIGHT PIPELINE
	VulkanPipelineCreateInfo brightPipelineInfo = rttPipelineInfo;
	brightPipelineInfo.fragmentShaderFilePath = "Shaders/Main_Shader.frag.spv";
	brightPipelineInfo.vertexShaderFilePath = "Shaders/Main_Shader.vert.spv";
	brightPipelineInfo.renderPass = &m_VulkanRenderPass->getHandles().brightRenderPass;
	brightPipelineInfo.descriptors = &m_BrightPipelineDescriptors;
	brightPipelineInfo.useVertexInput = false;
	brightPipelineInfo.msaaSamples = VK_SAMPLE_COUNT_1_BIT; 

	m_BrightVulkanPipeline = new VulkanPipeline(&brightPipelineInfo);
}


// Cập nhật Uniform Buffer Object (UBO) với ma trận camera hiện tại.
void Application::UpdateRTT_Uniforms()
{
	// Thiết lập view camera
	m_RTT_Ubo.view = glm::lookAt(glm::vec3(0.0f, 3.0f, 4.0f), glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	
	// Thiết lập ma trận projection
	m_RTT_Ubo.proj = glm::perspective(glm::radians(45.0f), m_VulkanSwapchain->getHandles().swapChainExtent.width / (float)m_VulkanSwapchain->getHandles().swapChainExtent.height, 0.1f, 10.0f);

	// GLM được thiết kế cho OpenGL, nơi tọa độ Y của clip coordinates bị đảo ngược.
	// Cách dễ nhất để sửa lỗi này là lật dấu của hệ số scale trên trục Y trong ma trận projection.
	m_RTT_Ubo.proj[1][1] *= -1;

	// Tải dữ liệu UBO lên buffer của GPU cho frame hiện tại
	m_RTT_UniformBuffers[m_CurrentFrame]->UploadData(&m_RTT_Ubo, sizeof(m_RTT_Ubo), 0);
}

// Cập nhật transform (vị trí, xoay, tỷ lệ) của các object trong scene.
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
	m_BunnyGirl->Scale({0.05f, 0.05f, 0.05f});
	
	m_Swimsuit->SetPosition({ -1, 0, 0 });
	m_Swimsuit->Scale({ 0.05f, 0.05f, 0.05f });
	m_Swimsuit->Rotate(glm::vec3(0, deltaTime * -45.0f, 0));
}

// Vòng lặp chính của ứng dụng.
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

// Thực hiện tất cả các hoạt động cần thiết để vẽ một frame.
void Application::DrawFrame()
{
	// 1. Đợi fence của frame hiện tại được báo hiệu (đảm bảo frame trước đã render xong)
	vkWaitForFences(m_VulkanContext->getVulkanHandles().device, 1, &m_VulkanSyncManager->getCurrentFence(m_CurrentFrame), VK_TRUE, UINT64_MAX);

	// 2. Lấy image có sẵn tiếp theo từ swapchain
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_VulkanContext->getVulkanHandles().device, m_VulkanSwapchain->getHandles().swapchain, UINT64_MAX,
		m_VulkanSyncManager->getCurrentImageAvailableSemaphore(m_CurrentFrame),
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

	// 3. Reset fence về trạng thái chưa được báo hiệu
	vkResetFences(m_VulkanContext->getVulkanHandles().device, 1, &m_VulkanSyncManager->getCurrentFence(m_CurrentFrame));

	// 4. Cập nhật dữ liệu động cho frame hiện tại
	UpdateRTT_Uniforms();
	UpdateRenderObjectTransforms();

	// 5. Ghi command buffer để vẽ
	vkResetCommandBuffer(m_VulkanCommandManager->getHandles().commandBuffers[m_CurrentFrame], 0);
	RecordCommandBuffer(m_VulkanCommandManager->getHandles().commandBuffers[m_CurrentFrame], imageIndex);

	// 6. Submit command buffer vào hàng đợi graphics
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_VulkanCommandManager->getHandles().commandBuffers[m_CurrentFrame];
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_VulkanSyncManager->getCurrentImageAvailableSemaphore(m_CurrentFrame); // Đợi cho đến khi image có sẵn
	submitInfo.pWaitDstStageMask = &waitStage;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_VulkanSyncManager->getCurrentRenderFinishedSemaphore(imageIndex); // Báo hiệu khi render xong

	VK_CHECK(vkQueueSubmit(m_VulkanContext->getVulkanHandles().graphicQueue, 1, &submitInfo, m_VulkanSyncManager->getCurrentFence(m_CurrentFrame)),
		"FAILED TO SUBMIT COMMAND BUFFER");

	// 7. Trình chiếu (present) image đã render ra màn hình
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_VulkanSwapchain->getHandles().swapchain;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_VulkanSyncManager->getCurrentRenderFinishedSemaphore(imageIndex); // Đợi render xong
	presentInfo.pImageIndices = &imageIndex;

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

// Ghi lại tất cả các lệnh render vào một command buffer.
void Application::RecordCommandBuffer(const VkCommandBuffer& cmdBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo cmdBeginInfo{};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmdBeginInfo), "FAILED TO BEGIN COMMAND BUFFER");

	// Bắt đầu render pass
	CmdDrawRTTRenderPass(cmdBuffer);
	CmdDrawBrightRenderPass(cmdBuffer);
	CmdDrawMainRenderPass(cmdBuffer, imageIndex);

	VK_CHECK(vkEndCommandBuffer(cmdBuffer), "FAILED TO END COMMAND BUFFER");
}

void Application::CmdDrawRTTRenderPass(const VkCommandBuffer& cmdBuffer)
{
	// Bắt đầu render pass
	VkRenderPassBeginInfo renderPassBeginInfo{};
	VkClearValue clearValues[3];
	clearValues[0].color = BACKGROUND_COLOR;
	clearValues[1].depthStencil = { 1.0f, 0 };
	clearValues[2].color = BACKGROUND_COLOR; // Dành cho target resolve của MSAA

	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.clearValueCount = 3;
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.framebuffer = m_RTT_FrameBuffers[m_CurrentFrame]->getFrameBuffer();
	renderPassBeginInfo.renderPass = m_VulkanRenderPass->getHandles().rttRenderPass;
	renderPassBeginInfo.renderArea.extent = m_VulkanSwapchain->getHandles().swapChainExtent;
	renderPassBeginInfo.renderArea.offset = { 0, 0 };

	vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Bind graphics pipeline
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_RTTVulkanPipeline->getHandles().pipeline);

	// Bind global vertex buffer và index buffer cho tất cả các mesh
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &m_MeshManager->getVertexBuffer(), &offset);
	vkCmdBindIndexBuffer(cmdBuffer, m_MeshManager->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

	// Bind descriptor sets cho shader (textures, uniforms)
	BindRTTDescriptorSets(cmdBuffer);

	// Gọi lệnh vẽ cho tất cả render object
	CmdDrawRTTRenderObjects(cmdBuffer);

	// Kết thúc render pass
	vkCmdEndRenderPass(cmdBuffer);
}

void Application::CmdDrawBrightRenderPass(const VkCommandBuffer& cmdBuffer)
{
	// Bắt đầu render pass
	VkRenderPassBeginInfo renderPassBeginInfo{};
	VkClearValue clearValues[1];
	clearValues[0].color = BACKGROUND_COLOR;

	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.framebuffer = m_Bright_FrameBuffers[m_CurrentFrame]->getFrameBuffer();
	renderPassBeginInfo.renderPass = m_VulkanRenderPass->getHandles().brightRenderPass;
	renderPassBeginInfo.renderArea.extent = m_VulkanSwapchain->getHandles().swapChainExtent;
	renderPassBeginInfo.renderArea.offset = { 0, 0 };

	vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


	// Bind graphics pipeline
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_BrightVulkanPipeline->getHandles().pipeline);

	// Bind descriptor sets cho shader (textures, uniforms)
	BindBrightDescriptorSets(cmdBuffer);

	// Gọi lệnh vẽ 
	CmdDrawBright(cmdBuffer);

	// Kết thúc render pass
	vkCmdEndRenderPass(cmdBuffer);
}

void Application::CmdDrawMainRenderPass(const VkCommandBuffer& cmdBuffer, uint32_t imageIndex)
{
	// Bắt đầu render pass
	VkRenderPassBeginInfo renderPassBeginInfo{};
	VkClearValue clearValues[3];
	clearValues[0].color = BACKGROUND_COLOR;
	clearValues[1].depthStencil = { 1.0f, 0 };
	clearValues[2].color = BACKGROUND_COLOR; // Dành cho target resolve của MSAA

	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.clearValueCount = 3;
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.framebuffer = m_Main_FrameBuffers[imageIndex]->getFrameBuffer();
	renderPassBeginInfo.renderPass = m_VulkanRenderPass->getHandles().mainRenderPass;
	renderPassBeginInfo.renderArea.extent = m_VulkanSwapchain->getHandles().swapChainExtent;
	renderPassBeginInfo.renderArea.offset = { 0, 0 };

	vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	

	// Bind graphics pipeline
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_MainVulkanPipeline->getHandles().pipeline);

	// Bind descriptor sets cho shader (textures, uniforms)
	BindMainDescriptorSets(cmdBuffer);

	// Gọi lệnh vẽ 
	CmdDrawMain(cmdBuffer);

	// Kết thúc render pass
	vkCmdEndRenderPass(cmdBuffer);
}

// Gọi lệnh vẽ cho mỗi mesh của mỗi render object.
void Application::CmdDrawRTTRenderObjects(const VkCommandBuffer& cmdBuffer)
{
	for (const auto& renderObject : m_RenderObjects)
	{
		std::vector<Mesh*> meshes = renderObject->getHandles().model->getMeshes();		
		for (const auto& mesh : meshes)
		{
			// Cập nhật push constants với dữ liệu của từng object (ma trận model, texture ID)
			m_PushConstantData.model = renderObject->GetModelMatrix();
			m_PushConstantData.textureId = mesh->textureId;
			
			vkCmdPushConstants(cmdBuffer, m_RTTVulkanPipeline->getHandles().pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m_PushConstantData), &m_PushConstantData);
			
			// Vẽ mesh theo index
			vkCmdDrawIndexed(cmdBuffer, mesh->meshRange.indexCount, 1, mesh->meshRange.firstIndex, mesh->meshRange.firstVertex, 0);
		}
	}
}

void Application::CmdDrawBright(const VkCommandBuffer& cmdBuffer)
{
	vkCmdDraw(cmdBuffer, 6, 1, 0, 0);
}

void Application::CmdDrawMain(const VkCommandBuffer& cmdBuffer)
{
	vkCmdDraw(cmdBuffer, 6, 1, 0, 0);
}

// Bind các descriptor set cần thiết vào pipeline.
void Application::BindRTTDescriptorSets(const VkCommandBuffer& cmdBuffer)
{
	// Bind descriptor set cho texture images (Set 0)
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_RTTVulkanPipeline->getHandles().pipelineLayout,
		m_TextureManager->getDescriptor()->getHandles().setIndex, 1,
		&m_TextureManager->getDescriptor()->getHandles().descriptorSet,
		0, nullptr);
	
	// Bind descriptor set cho uniform buffer (Set 1) của frame hiện tại
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_RTTVulkanPipeline->getHandles().pipelineLayout,
		m_RTT_UniformDescriptors[m_CurrentFrame]->getSetIndex() , 1,
		&m_RTT_UniformDescriptors[m_CurrentFrame]->getHandles().descriptorSet,
		0, nullptr);
}

void Application::BindMainDescriptorSets(const VkCommandBuffer& cmdBuffer)
{
	// Bind Texture Descriptor (Set 0)
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_MainVulkanPipeline->getHandles().pipelineLayout,
			m_MainTextureDescriptors[m_CurrentFrame]->getSetIndex(), 1,
			&m_MainTextureDescriptors[m_CurrentFrame]->getHandles().descriptorSet,
			0, nullptr);
	}
}

void Application::BindBrightDescriptorSets(const VkCommandBuffer& cmdBuffer)
{
	// Bind Texture Descriptor (Set 0)
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_BrightVulkanPipeline->getHandles().pipelineLayout,
			m_BrightTextureDescriptors[m_CurrentFrame]->getSetIndex(), 1,
			&m_BrightTextureDescriptors[m_CurrentFrame]->getHandles().descriptorSet,
			0, nullptr);
	}
}
