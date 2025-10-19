#pragma once
#include "Core/Window.h"
#include "Core/VulkanContext.h"
#include "Core/VulkanSwapchain.h"
#include "Core/VulkanImage.h"
#include "Core/VulkanRenderPass.h"
#include "Core/VulkanFrameBuffer.h"
#include "Core/VulkanCommandManager.h"
#include "Core/VulkanPipeline.h"
#include "Core/VulkanSyncManager.h"

class Application
{
public:
	Application();
	~Application();

	void Loop();
private:
	const uint32_t WINDOW_WIDTH = 800;
	const uint32_t WINDOW_HEIGHT = 600;

	const VkClearColorValue backgroundColor = { 0.1f, 0.1f, 0.2f, 1.0f };

	const VkSampleCountFlagBits MSAA_SAMPLES = VK_SAMPLE_COUNT_2_BIT;
	const int MAX_FRAMES_IN_FLIGHT = 2;

	int currentFrame = 0;

	Window* window;
	VulkanContext* vulkanContext;
	VulkanSwapchain* vulkanSwapchain;
	VulkanRenderPass* vulkanRenderPass;
	VulkanFrameBuffer* vulkanFrameBuffer;

	VulkanCommandManager* vulkanCommandManager;
	VulkanPipeline* vulkanPipeline;
	VulkanSyncManager* vulkanSyncManager;

	VulkanHandles vulkanHandles;
	SwapchainHandles swapchainHandles;
	RenderPassHandles renderPassHandles;
	FrameBufferHandles frameBufferHandles;
	CommandManagerHandles commandManagerHandles;
	PipelineHandles pipelineHandles;

	void CreateCacheHandles();


	void RecordCommandBuffer(VkCommandBuffer cmdBuffer, uint32_t imageIndex);
	void DrawFrame();
};
