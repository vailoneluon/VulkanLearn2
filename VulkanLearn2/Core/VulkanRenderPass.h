#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanSwapchain.h"
#include "VulkanContext.h"

using namespace std;

struct RenderPassHandles
{
	VkRenderPass renderPass;
};

class VulkanRenderPass
{
public:
	VulkanRenderPass(const VulkanHandles& vulkanHandles, const SwapchainHandles& swapchainHandles, VkSampleCountFlagBits msaaSampler);
	~VulkanRenderPass();

	const RenderPassHandles& getHandles() const { return handles; }

private:
	RenderPassHandles handles;
	const VulkanHandles& vk;
	const SwapchainHandles& sc;
};
