#pragma once
#include "VulkanContext.h"

struct SwapchainHandles;

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
};
