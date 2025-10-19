#include "main.h"
#include "Utils/ErrorHelper.h" 
#include <stdexcept>          

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

	vulkanPipeline = new VulkanPipeline(
		vulkanContext->getVulkanHandles(),
		vulkanRenderPass->getHandles(),
		vulkanSwapchain->getHandles(),
		MSAA_SAMPLES);

	vulkanSyncManager = new VulkanSyncManager(vulkanContext->getVulkanHandles(), MAX_FRAMES_IN_FLIGHT, vulkanSwapchain->getHandles().swapchainImageCount);

	CreateCacheHandles();
}


void Application::CreateCacheHandles()
{
	vulkanHandles = vulkanContext->getVulkanHandles();
	swapchainHandles = vulkanSwapchain->getHandles();
	renderPassHandles = vulkanRenderPass->getHandles();
	frameBufferHandles = vulkanFrameBuffer->getHandles();
	commandManagerHandles = vulkanCommandManager->getHandles();
	pipelineHandles = vulkanPipeline->getHandles();
}

Application::~Application()
{
	vkDeviceWaitIdle(vulkanHandles.device);

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

	vkDeviceWaitIdle(vulkanHandles.device);
}

void Application::DrawFrame()
{
	vkWaitForFences(vulkanHandles.device, 1, &vulkanSyncManager->getCurrentFence(currentFrame), VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(vulkanHandles.device, swapchainHandles.swapchain, UINT64_MAX,
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

	vkResetFences(vulkanHandles.device, 1, &vulkanSyncManager->getCurrentFence(currentFrame));

	vkResetCommandBuffer(commandManagerHandles.commandBuffers[currentFrame], 0);
	RecordCommandBuffer(commandManagerHandles.commandBuffers[currentFrame], imageIndex);

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandManagerHandles.commandBuffers[currentFrame];

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &vulkanSyncManager->getCurrentImageAvailableSemaphore(currentFrame);
	submitInfo.pWaitDstStageMask = &waitStage;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &vulkanSyncManager->getCurrentRenderFinishedSemaphore(imageIndex);

	VK_CHECK(vkQueueSubmit(vulkanHandles.graphicQueue, 1, &submitInfo, vulkanSyncManager->getCurrentFence(currentFrame)),
		"FAILED TO SUBMIT COMMAND BUFFER");


	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchainHandles.swapchain;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &vulkanSyncManager->getCurrentRenderFinishedSemaphore(imageIndex);
	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(vulkanHandles.presentQueue, &presentInfo);

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
	renderPassBeginInfo.framebuffer = frameBufferHandles.frameBuffers[imageIndex];
	renderPassBeginInfo.renderPass = renderPassHandles.renderPass;
	renderPassBeginInfo.renderArea.extent = swapchainHandles.swapChainExtent;
	renderPassBeginInfo.renderArea.offset = { 0, 0 };

	vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineHandles.graphicsPipeline);

	vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(cmdBuffer);

	VK_CHECK(vkEndCommandBuffer(cmdBuffer), "FAILED TO END COMMAND BUFFER");
}