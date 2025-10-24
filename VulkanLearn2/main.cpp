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

int main()
{
	Application app;

	app.Loop();
}

Application::Application()
{
	window = new Window(800, 600, "ZOLCOL VULKAN");

	vulkanContext = new VulkanContext(window->getGLFWWindow(), window->getInstanceExtensionsRequired());
	vulkanSwapchain = new VulkanSwapchain(vulkanContext->getVulkanHandles(), window->getGLFWWindow());
	vulkanRenderPass = new VulkanRenderPass(vulkanContext->getVulkanHandles(), vulkanSwapchain->getHandles(), MSAA_SAMPLES);
	vulkanFrameBuffer = new VulkanFrameBuffer(vulkanContext->getVulkanHandles(), vulkanSwapchain->getHandles(), vulkanRenderPass->getHandles(), MSAA_SAMPLES);

	vulkanCommandManager = new VulkanCommandManager(vulkanContext->getVulkanHandles(), MAX_FRAMES_IN_FLIGHT);

	vulkanSampler = new VulkanSampler(vulkanContext->getVulkanHandles());
	CreateUniformBuffer();
	CreateTextureImage(vulkanContext->getVulkanHandles());

	vulkanDescriptorManager = new VulkanDescriptorManager(vulkanContext->getVulkanHandles(), descriptors);

	vulkanPipeline = new VulkanPipeline(
		vulkanContext->getVulkanHandles(),
		vulkanRenderPass->getHandles(),
		vulkanSwapchain->getHandles(),
		MSAA_SAMPLES,
		descriptors);


	vulkanSyncManager = new VulkanSyncManager(vulkanContext->getVulkanHandles(), MAX_FRAMES_IN_FLIGHT, vulkanSwapchain->getHandles().swapchainImageCount);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	CreateVertexBuffer();
	CreateIndexBuffer();

}

void Application::CreateVertexBuffer()
{
	uint32_t bufferSize = vertices.size() * sizeof(vertices[0]);

	VkBufferCreateInfo bufferInfo{};

	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.queueFamilyIndexCount = 1;
	bufferInfo.pQueueFamilyIndices = &vulkanContext->getVulkanHandles().queueFamilyIndices.GraphicQueueIndex;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	vertexBuffer = new VulkanBuffer(vulkanContext->getVulkanHandles(), vulkanCommandManager, bufferInfo, true);

	vertexBuffer->UploadData(vertices.data(), bufferSize, 0);
}

void Application::CreateIndexBuffer()
{
	uint32_t bufferSize = indices.size() * sizeof(indices[0]);

	VkBufferCreateInfo bufferInfo{};

	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.queueFamilyIndexCount = 1;
	bufferInfo.pQueueFamilyIndices = &vulkanContext->getVulkanHandles().queueFamilyIndices.GraphicQueueIndex;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

	indexBuffer = new VulkanBuffer(vulkanContext->getVulkanHandles(), vulkanCommandManager, bufferInfo, true);

	indexBuffer->UploadData(indices.data(), bufferSize, 0);
}

void Application::CreateTextureImage(const VulkanHandles& vk)
{
	textureImage = new VulkanImage(vulkanContext->getVulkanHandles(), vulkanCommandManager, "Images/image.jpg", true);

	///////////////////////////////////////
	VkDescriptorImageInfo textureDescImageInfo{};
	textureDescImageInfo.sampler = vulkanSampler->getSampler();
	textureDescImageInfo.imageView = textureImage->getHandles().imageView;
	textureDescImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	BindingElementInfo textureImageElementInfo;
	textureImageElementInfo.binding = 0;
	textureImageElementInfo.descriptorCount = 1;
	textureImageElementInfo.pImmutableSamplers = nullptr;
	textureImageElementInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureImageElementInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	textureImageElementInfo.imageDataInfo = textureDescImageInfo;

	vector<BindingElementInfo> textureBindings{ textureImageElementInfo };
	textureImageDescriptor = new VulkanDescriptor(vk, textureBindings);
	descriptors.push_back(textureImageDescriptor);
	//
	uniformDescriptors.resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorBufferInfo uniformBufferInfo{};
		uniformBufferInfo.buffer = uniformBuffers[i]->getHandles().buffer;
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = uniformBuffers[i]->getHandles().bufferSize;

		BindingElementInfo uniformElementInfo;
		uniformElementInfo.binding = 0;
		uniformElementInfo.pImmutableSamplers = nullptr;
		uniformElementInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformElementInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uniformElementInfo.descriptorCount = 1;
		uniformElementInfo.bufferDataInfo = uniformBufferInfo;

		vector<BindingElementInfo> uniformBindings{ uniformElementInfo };

		uniformDescriptors[i] = new VulkanDescriptor(vk, uniformBindings);
		descriptors.push_back(uniformDescriptors[i]);
	}
}

void Application::CreateUniformBuffer()
{
	uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.queueFamilyIndexCount = 1;
	bufferInfo.pQueueFamilyIndices = &vulkanContext->getVulkanHandles().queueFamilyIndices.GraphicQueueIndex;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.size = sizeof(UniformBufferObject);
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		uniformBuffers[i] = new VulkanBuffer(vulkanContext->getVulkanHandles(), vulkanCommandManager, bufferInfo, false);
	}
}

void Application::UpdateUniforms()
{
	static auto startTime = chrono::high_resolution_clock::now();

	auto currentTime = chrono::high_resolution_clock::now();
	float time = chrono::duration<float, chrono::seconds::period>(currentTime - startTime).count();

	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.view = glm::lookAt(glm::vec3(0.0f, 2.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), vulkanSwapchain->getHandles().swapChainExtent.width / (float)vulkanSwapchain->getHandles().swapChainExtent.height, 0.1f, 10.0f);

	ubo.proj[1][1] *= -1;

	uniformBuffers[currentFrame]->UploadData(&ubo, sizeof(ubo), 0);
}

Application::~Application()
{
	vkDeviceWaitIdle(vulkanContext->getVulkanHandles().device);


	delete(vertexBuffer);
	delete(indexBuffer);
	delete(vulkanSampler);


	delete(textureImage);

	for (auto uniformBuffer : uniformBuffers)
	{
		delete(uniformBuffer);
	}

	delete(vulkanDescriptorManager);

	delete(vulkanSyncManager);
	delete(vulkanPipeline);
	delete(vulkanCommandManager);
	delete(vulkanFrameBuffer);
	delete(vulkanRenderPass);
	delete(vulkanSwapchain);
	delete(vulkanContext);
	delete(window);
}

void Application::Loop()
{
	while (!window->windowShouldClose())
	{
		window->windowPollEvents();
		DrawFrame();
	}

	vkDeviceWaitIdle(vulkanContext->getVulkanHandles().device);
}

void Application::DrawFrame()
{
	vkWaitForFences(vulkanContext->getVulkanHandles().device, 1, &vulkanSyncManager->getCurrentFence(currentFrame), VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(vulkanContext->getVulkanHandles().device, vulkanSwapchain->getHandles().swapchain, UINT64_MAX,
		vulkanSyncManager->getCurrentImageAvailableSemaphore(currentFrame),
		VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw runtime_error("FAILED TO ACQUIRE SWAPCHAIN IMAGE");
	}

	vkResetFences(vulkanContext->getVulkanHandles().device, 1, &vulkanSyncManager->getCurrentFence(currentFrame));

	// Update Data For Frame
	UpdateUniforms();

	vkResetCommandBuffer(vulkanCommandManager->getHandles().commandBuffers[currentFrame], 0);
	RecordCommandBuffer(vulkanCommandManager->getHandles().commandBuffers[currentFrame], imageIndex);

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vulkanCommandManager->getHandles().commandBuffers[currentFrame];

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &vulkanSyncManager->getCurrentImageAvailableSemaphore(currentFrame);
	submitInfo.pWaitDstStageMask = &waitStage;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &vulkanSyncManager->getCurrentRenderFinishedSemaphore(imageIndex);

	VK_CHECK(vkQueueSubmit(vulkanContext->getVulkanHandles().graphicQueue, 1, &submitInfo, vulkanSyncManager->getCurrentFence(currentFrame)),
		"FAILED TO SUBMIT COMMAND BUFFER");


	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &vulkanSwapchain->getHandles().swapchain;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &vulkanSyncManager->getCurrentRenderFinishedSemaphore(imageIndex);
	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(vulkanContext->getVulkanHandles().presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
	}
	else if (result != VK_SUCCESS)
	{
		throw runtime_error("FAILED TO PRESENT SWAPCHAIN IMAGE");
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::RecordCommandBuffer(VkCommandBuffer cmdBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo cmdBeginInfo{};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmdBeginInfo), "FAILED TO BEGIN COMMAND BUFFER");

	VkRenderPassBeginInfo renderPassBeginInfo{};

	VkClearValue clearValues[3];
	clearValues[0].color = backgroundColor;
	clearValues[1].depthStencil = { 1.0f, 0 };
	clearValues[2].color = backgroundColor;

	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.clearValueCount = 3;
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.framebuffer = vulkanFrameBuffer->getHandles().frameBuffers[imageIndex];
	renderPassBeginInfo.renderPass = vulkanRenderPass->getHandles().renderPass;
	renderPassBeginInfo.renderArea.extent = vulkanSwapchain->getHandles().swapChainExtent;
	renderPassBeginInfo.renderArea.offset = { 0, 0 };

	vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->getHandles().graphicsPipeline);

	// Bind VertexBuffer and IndexBuffer
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer->getHandles().buffer, &offset);
	vkCmdBindIndexBuffer(cmdBuffer, indexBuffer->getHandles().buffer, 0, VK_INDEX_TYPE_UINT16);

	// Bind Descriptor Set
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		vulkanPipeline->getHandles().pipelineLayout,
		0, 1,
		&textureImageDescriptor->getHandles().descriptorSet,
		0, nullptr);

	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		vulkanPipeline->getHandles().pipelineLayout,
		1, 1,
		&uniformDescriptors[currentFrame]->getHandles().descriptorSet,
		0, nullptr);

	vkCmdDrawIndexed(cmdBuffer, indices.size(), 1, 0, 0, 0);

	vkCmdEndRenderPass(cmdBuffer);

	VK_CHECK(vkEndCommandBuffer(cmdBuffer), "FAILED TO END COMMAND BUFFER");
}