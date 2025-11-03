#include "pch.h"
#include "main.h"

#include "Core/Window.h"
#include "Core/VulkanSwapchain.h"
#include "Core/VulkanImage.h"
#include "Core/VulkanRenderPass.h"
#include "Core/VulkanFrameBuffer.h"
#include "Core/VulkanCommandManager.h"
#include "Core/VulkanPipeline.h"
#include "Core/VulkanSyncManager.h"
#include "Core/VulkanBuffer.h"
#include "Core/VulkanDescriptorManager.h"
#include "Core/VulkanSampler.h"
#include "Core/VulkanDescriptor.h"

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
	m_VulkanFrameBuffer = new VulkanFrameBuffer(m_VulkanContext->getVulkanHandles(), m_VulkanSwapchain->getHandles(), m_VulkanRenderPass->getHandles(), MSAA_SAMPLES);
	m_VulkanCommandManager = new VulkanCommandManager(m_VulkanContext->getVulkanHandles(), MAX_FRAMES_IN_FLIGHT);
	m_VulkanSampler = new VulkanSampler(m_VulkanContext->getVulkanHandles());
	
	// 3. Tạo uniform buffers, một cái cho mỗi frame-in-flight
	CreateUniformBuffers();
	CreateMainDescriptors();

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
	m_allDescriptors.insert(m_allDescriptors.begin(), m_RTTPipelineDescriptors.begin(), m_RTTPipelineDescriptors.end());
	m_allDescriptors.insert(m_allDescriptors.begin(), m_MainPipelineDescriptors.begin(), m_MainPipelineDescriptors.end());
	m_VulkanDescriptorManager = new VulkanDescriptorManager(m_VulkanContext->getVulkanHandles(), m_allDescriptors);
	UpdateRTTDescriptorBindings();
	UpdateMainDescriptorBindings();

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

	// Thứ tự xóa rất quan trọng. Thường là ngược lại với thứ tự tạo.
	for (auto& renderObject : m_RenderObjects)
	{
		delete(renderObject);
	}

	delete(m_MeshManager);
	delete(m_TextureManager);
	delete(m_VulkanSampler);

	for (auto& uniformBuffer : m_UniformBuffers)
	{
		delete(uniformBuffer);
	}

	delete(m_VulkanDescriptorManager);
	delete(m_VulkanSyncManager);
	delete(m_RTTVulkanPipeline);
	delete(m_MainVulkanPipeline);
	delete(m_VulkanCommandManager);
	delete(m_VulkanFrameBuffer);
	delete(m_VulkanRenderPass);
	delete(m_VulkanSwapchain);
	delete(m_VulkanContext);
	delete(m_Window);
}

// Tạo uniform buffers cho mỗi frame-in-flight để lưu trữ ma trận camera.
void Application::CreateUniformBuffers()
{
	m_UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	m_UniformDescriptors.resize(MAX_FRAMES_IN_FLIGHT);

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
		m_UniformBuffers[i] = new VulkanBuffer(m_VulkanContext->getVulkanHandles(), m_VulkanCommandManager, bufferInfo, VMA_MEMORY_USAGE_CPU_TO_GPU);

		// Tạo descriptor cho buffer
		BindingElementInfo uniformElementInfo;
		uniformElementInfo.binding = 0;
		uniformElementInfo.pImmutableSamplers = nullptr;
		uniformElementInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformElementInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uniformElementInfo.descriptorCount = 1;
		std::vector<BindingElementInfo> uniformBindings{ uniformElementInfo };
		m_UniformDescriptors[i] = new VulkanDescriptor(m_VulkanContext->getVulkanHandles(), uniformBindings, 1);
		
		// Thêm descriptor vào danh sách tất cả descriptor để tạo pipeline
		m_RTTPipelineDescriptors.push_back(m_UniformDescriptors[i]);
	}
}

void Application::CreateMainDescriptors()
{
	m_MainPipelineDescriptors.resize(MAX_FRAMES_IN_FLIGHT);

	BindingElementInfo bindingElement{};
	bindingElement.binding = 0;
	bindingElement.descriptorCount = 1;

	bindingElement.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindingElement.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::vector<BindingElementInfo> bindingElements = {bindingElement};

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_MainPipelineDescriptors[i] = new VulkanDescriptor(m_VulkanContext->getVulkanHandles(), bindingElements, 0);
	}
}

void Application::CreatePipelines()
{
	VulkanPipelineCreateInfo rttPipelineInfo{};
	rttPipelineInfo.descriptors = &m_RTTPipelineDescriptors;
	rttPipelineInfo.fragmentShaderFilePath = "Shaders/frag.spv";
	rttPipelineInfo.vertexShaderFilePath = "Shaders/vert.spv";
	rttPipelineInfo.msaaSamples = MSAA_SAMPLES;
	rttPipelineInfo.renderPass = &m_VulkanRenderPass->getHandles().rttRenderPass;
	rttPipelineInfo.swapchainHandles = &m_VulkanSwapchain->getHandles();
	rttPipelineInfo.vulkanHandles = &m_VulkanContext->getVulkanHandles();

	m_RTTVulkanPipeline = new VulkanPipeline(&rttPipelineInfo);
	
	VulkanPipelineCreateInfo mainPipelineInfo = rttPipelineInfo;
	mainPipelineInfo.fragmentShaderFilePath = "Shaders/mainFrag.spv";
	mainPipelineInfo.vertexShaderFilePath = "Shaders/mainVert.spv";
	mainPipelineInfo.renderPass = &m_VulkanRenderPass->getHandles().mainRenderPass;
	mainPipelineInfo.descriptors = &m_MainPipelineDescriptors;
	mainPipelineInfo.useVertexInput = false;

	m_MainVulkanPipeline = new VulkanPipeline(&mainPipelineInfo);
}

// Cập nhật descriptor sets để trỏ đến đúng tài nguyên buffer/image.
void Application::UpdateRTTDescriptorBindings()
{
	// Cập nhật descriptor của texture
	m_TextureManager->UpdateTextureImageDescriptorBinding();

	// Cập nhật descriptor của uniform buffer cho mỗi frame
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorBufferInfo uniformBufferInfo{};
		uniformBufferInfo.buffer = m_UniformBuffers[i]->GetHandles().buffer;
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = m_UniformBuffers[i]->GetHandles().bufferSize;

		BufferDescriptorUpdateInfo bufferBindingInfo{};
		bufferBindingInfo.binding = 0;
		bufferBindingInfo.bufferInfoCount = 1;
		bufferBindingInfo.bufferInfos = &uniformBufferInfo;
		bufferBindingInfo.firstArrayElement = 0;

		m_UniformDescriptors[i]->WriteBufferSets(1, &bufferBindingInfo);
	}
}

void Application::UpdateMainDescriptorBindings()
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_VulkanFrameBuffer->getHandles().rttColorImages[i]->GetHandles().imageView;
		imageInfo.sampler = m_VulkanSampler->getSampler();
	
		ImageDescriptorUpdateInfo imageUpdateInfo{};
		imageUpdateInfo.binding = 0;
		imageUpdateInfo.firstArrayElement = 0;
		imageUpdateInfo.imageInfoCount = 1;
		imageUpdateInfo.imageInfos = &imageInfo;

		m_MainPipelineDescriptors[i]->WriteImageSets(1, &imageUpdateInfo);
	}
}

// Cập nhật Uniform Buffer Object (UBO) với ma trận camera hiện tại.
void Application::UpdateUniforms()
{
	// Thiết lập view camera
	m_Ubo.view = glm::lookAt(glm::vec3(0.0f, 3.0f, 4.0f), glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	
	// Thiết lập ma trận projection
	m_Ubo.proj = glm::perspective(glm::radians(45.0f), m_VulkanSwapchain->getHandles().swapChainExtent.width / (float)m_VulkanSwapchain->getHandles().swapChainExtent.height, 0.1f, 10.0f);

	// GLM được thiết kế cho OpenGL, nơi tọa độ Y của clip coordinates bị đảo ngược.
	// Cách dễ nhất để sửa lỗi này là lật dấu của hệ số scale trên trục Y trong ma trận projection.
	m_Ubo.proj[1][1] *= -1;

	// Tải dữ liệu UBO lên buffer của GPU cho frame hiện tại
	m_UniformBuffers[m_CurrentFrame]->UploadData(&m_Ubo, sizeof(m_Ubo), 0);
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
	UpdateUniforms();
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

	// transition image layout
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = m_VulkanFrameBuffer->getHandles().rttColorImages[m_CurrentFrame]->GetHandles().image;
	barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseMipLevel = 0;

	vkCmdPipelineBarrier(cmdBuffer,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, 
		0, nullptr, 
		0, nullptr, 
		1, &barrier);

	CmdDrawMainRenderPass(cmdBuffer, imageIndex);

	VK_CHECK(vkEndCommandBuffer(cmdBuffer), "FAILED TO END COMMAND BUFFER");
}

void Application::CmdDrawRTTRenderPass(const VkCommandBuffer& cmdBuffer)
{
	// Bắt đầu render pass
	VkRenderPassBeginInfo renderPassBeginInfo{};
	VkClearValue clearValues[2];
	clearValues[0].color = BACKGROUND_COLOR;
	clearValues[1].depthStencil = { 1.0f, 0 };
	//clearValues[2].color = BACKGROUND_COLOR; // Dành cho target resolve của MSAA

	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.framebuffer = m_VulkanFrameBuffer->getHandles().rttFrameBuffers[m_CurrentFrame];
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
	renderPassBeginInfo.framebuffer = m_VulkanFrameBuffer->getHandles().mainFrameBuffers[imageIndex];
	renderPassBeginInfo.renderPass = m_VulkanRenderPass->getHandles().mainRenderPass;
	renderPassBeginInfo.renderArea.extent = m_VulkanSwapchain->getHandles().swapChainExtent;
	renderPassBeginInfo.renderArea.offset = { 0, 0 };

	vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	

	// Bind graphics pipeline
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_MainVulkanPipeline->getHandles().pipeline);

	// Bind global vertex buffer và index buffer cho tất cả các mesh
	//VkDeviceSize offset = 0;
	//vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &m_MeshManager->getVertexBuffer(), &offset);
	//vkCmdBindIndexBuffer(cmdBuffer, m_MeshManager->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

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
		m_UniformDescriptors[m_CurrentFrame]->getSetIndex() , 1,
		&m_UniformDescriptors[m_CurrentFrame]->getHandles().descriptorSet,
		0, nullptr);
}

void Application::BindMainDescriptorSets(const VkCommandBuffer& cmdBuffer)
{
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_MainVulkanPipeline->getHandles().pipelineLayout,
		m_MainPipelineDescriptors[m_CurrentFrame]->getSetIndex(), 1,
		&m_MainPipelineDescriptors[m_CurrentFrame]->getHandles().descriptorSet,
		0, nullptr);
}
