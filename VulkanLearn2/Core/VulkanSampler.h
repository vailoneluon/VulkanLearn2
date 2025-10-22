#pragma once
#include <vulkan/vulkan.h>
#include "VulkanContext.h"

struct SamplerHandles
{
	VkSampler sampler;
};

class VulkanSampler
{
public:
	VulkanSampler(const VulkanHandles& vulkanHandles);
	~VulkanSampler();

	const VkSampler& getSampler() { return handles.sampler; };

private:
	const VulkanHandles& vk;

	SamplerHandles handles;
};
