#pragma once
#include "Core/Window.h"
#include "Core/VulkanContext.h"
#include "Core/VulkanSwapchain.h"
#include "Core/VulkanImage.h"
#include "Core/VulkanRenderPass.h"
#include "Core/VulkanFrameBuffer.h"
#include "Core/VulkanCommandManager.h"
#include "Core/VulkanPipeline.h"

class Application
{
public:
	Application();
	~Application();

	void Loop();
private:
	const uint32_t WINDOW_WIDTH = 800;
	const uint32_t WINDOW_HEIGHT = 600;

	const VkSampleCountFlagBits MSAA_SAMPLES = VK_SAMPLE_COUNT_2_BIT;
	const int MAX_FRAMES_IN_FLIGHT = 2;

	Window* window;
	VulkanContext* vulkanContext;
	VulkanSwapchain* vulkanSwapchain;
	VulkanRenderPass* vulkanRenderPass;
	VulkanFrameBuffer* vulkanFrameBuffer;

	VulkanCommandManager* vulkanCommandManager;

	VulkanPipeline* vulkanPipeline;
};
