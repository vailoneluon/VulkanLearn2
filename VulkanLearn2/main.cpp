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

	/////////////////////////
	meshManager = new MeshManager(vulkanContext->getVulkanHandles(), vulkanCommandManager);
	textureManager = new TextureManager(vulkanContext->getVulkanHandles(), vulkanCommandManager, vulkanSampler->getSampler());

	obj1 = new RenderObject("Resources/bunnyGirl.assbin", meshManager, textureManager);
	obj2 = new RenderObject("Resources/swimSuit.assbin", meshManager, textureManager);
	obj1->SetRotation({ -90, 0, 0 });
	//obj2->SetRotation({ -90, 0, 0 });
	renderObjects.push_back(obj1);
	renderObjects.push_back(obj2);

	textureManager->FinalizeSetup();
	descriptors.push_back(textureManager->getDescriptor());

	/////////////////

	vulkanDescriptorManager = new VulkanDescriptorManager(vulkanContext->getVulkanHandles(), descriptors);
	UpdateDescriptorBinding();


	vulkanPipeline = new VulkanPipeline(
		vulkanContext->getVulkanHandles(),
		vulkanRenderPass->getHandles(),
		vulkanSwapchain->getHandles(),
		MSAA_SAMPLES,
		descriptors);


	vulkanSyncManager = new VulkanSyncManager(vulkanContext->getVulkanHandles(), MAX_FRAMES_IN_FLIGHT, vulkanSwapchain->getHandles().swapchainImageCount);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	meshManager->CreateBuffers();
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

	uniformDescriptors.resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		uniformBuffers[i] = new VulkanBuffer(vulkanContext->getVulkanHandles(), vulkanCommandManager, bufferInfo, false);

		BindingElementInfo uniformElementInfo;
		uniformElementInfo.binding = 0;
		uniformElementInfo.pImmutableSamplers = nullptr;
		uniformElementInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformElementInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uniformElementInfo.descriptorCount = 1;

		std::vector<BindingElementInfo> uniformBindings{ uniformElementInfo };

		uniformDescriptors[i] = new VulkanDescriptor(vulkanContext->getVulkanHandles(), uniformBindings, 1);
		descriptors.push_back(uniformDescriptors[i]);
	}
}

void Application::UpdateDescriptorBinding()
{
	textureManager->UpdateTextureImageDescriptorBinding();

	// UBO Binding
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorBufferInfo uniformBufferInfo{};
		uniformBufferInfo.buffer = uniformBuffers[i]->getHandles().buffer;
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = uniformBuffers[i]->getHandles().bufferSize;

		BufferBindingUpdateInfo bufferBindingInfo{};
		bufferBindingInfo.binding = 0;
		bufferBindingInfo.bufferInfo = uniformBufferInfo;

		uniformDescriptors[i]->UpdateBufferBinding(1, &bufferBindingInfo);
	}
}

void Application::UpdateUniforms()
{
	/*static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();*/

	ubo.view = glm::lookAt(glm::vec3(0.0f, 3.0f, 4.0f), glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), vulkanSwapchain->getHandles().swapChainExtent.width / (float)vulkanSwapchain->getHandles().swapChainExtent.height, 0.1f, 10.0f);

	ubo.proj[1][1] *= -1;

	uniformBuffers[currentFrame]->UploadData(&ubo, sizeof(ubo), 0);
}

void Application::UpdateRenderObjectTransform()
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	static auto lastTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
	lastTime = currentTime;

	obj1->SetPosition({ 1, 0, 0 });
	obj1->Rotate(glm::vec3(0, deltaTime * 45.0f, 0));
	obj1->Scale({0.05f, 0.05f, 0.05f});
	
	obj2->SetPosition({ -1, 0, 0 });
	obj2->Scale({ 0.05f, 0.05f, 0.05f });
	obj2->Rotate(glm::vec3(0, deltaTime * -45.0f, 0));
}

Application::~Application()
{
	vkDeviceWaitIdle(vulkanContext->getVulkanHandles().device);

	for (auto& renderObject : renderObjects)
	{
		delete(renderObject);
	}

	delete(meshManager);
	delete(textureManager);
	delete(vulkanSampler);


	for (auto& uniformBuffer : uniformBuffers)
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
		throw std::runtime_error("FAILED TO ACQUIRE SWAPCHAIN IMAGE");
	}

	vkResetFences(vulkanContext->getVulkanHandles().device, 1, &vulkanSyncManager->getCurrentFence(currentFrame));

	// Update Data For Frame
	UpdateUniforms();
	UpdateRenderObjectTransform();

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
		throw std::runtime_error("FAILED TO PRESENT SWAPCHAIN IMAGE");
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::RecordCommandBuffer(const VkCommandBuffer& cmdBuffer, uint32_t imageIndex)
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
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &meshManager->getVertexBuffer(), &offset);
	vkCmdBindIndexBuffer(cmdBuffer, meshManager->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

	// Bind Descriptor Set
	BindDescriptorSet(cmdBuffer);
	
	CmdDrawRenderObjects(cmdBuffer);

	vkCmdEndRenderPass(cmdBuffer);

	VK_CHECK(vkEndCommandBuffer(cmdBuffer), "FAILED TO END COMMAND BUFFER");
}

void Application::CmdDrawRenderObjects(const VkCommandBuffer& cmdBuffer)
{
	for (const auto& renderObject : renderObjects)
	{
		

		std::vector<Mesh*> meshes = renderObject->getHandles().model->getMeshes();		
		for (const auto& mesh : meshes)
		{
			// Push Constant Data
			pushConstantData.model = renderObject->GetModelMatrix();
			pushConstantData.textureId = mesh->textureId;
			
			vkCmdPushConstants(cmdBuffer, vulkanPipeline->getHandles().pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstantData), &pushConstantData);
			vkCmdDrawIndexed(cmdBuffer, mesh->meshRange.indexCount, 1, mesh->meshRange.firstIndex, mesh->meshRange.firstVertex, 0);
		}
	}
}

void Application::BindDescriptorSet(const VkCommandBuffer& cmdBuffer)
{
	// Binding Texture Image Descriptor Set
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		vulkanPipeline->getHandles().pipelineLayout,
		textureManager->getDescriptor()->getHandles().setIndex, 1,
		&textureManager->getDescriptor()->getHandles().descriptorSet,
		0, nullptr);
	
	// Binding UBO Descriptor Set
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		vulkanPipeline->getHandles().pipelineLayout,
		uniformDescriptors[currentFrame]->getSetIndex() , 1,
		&uniformDescriptors[currentFrame]->getHandles().descriptorSet,
		0, nullptr);
}
