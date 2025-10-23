#pragma once
#include <vulkan/vulkan.h>
#include "VulkanContext.h"

struct DescriptorHandles
{
	
};


class DescriptorInfo
{
public:
	DescriptorInfo(const VulkanHandles& vulkanHandles);
	~DescriptorInfo();


private:
	const VulkanHandles& vk;

};
