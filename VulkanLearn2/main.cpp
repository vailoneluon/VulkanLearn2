#include "main.h"

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
}

Application::~Application()
{

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
	}
}
