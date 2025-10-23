#pragma once
#include <vulkan/vulkan.h>
#include "VulkanContext.h"


struct DescriptorSetLayoutHandles
{
	VkDescriptorSetLayout descSetLayout;
};

class VulkanDescriptorSetLayout
{
public:
	VulkanDescriptorSetLayout(const VulkanHandles& vulkanHandles, const vector<VkDescriptorSetLayoutBinding>& layoutBindingList);
	~VulkanDescriptorSetLayout();

	const VkDescriptorSetLayout& getDescriptorSetLayout() const { return handles.descSetLayout; }

private:
	const VulkanHandles& vk;
	
	DescriptorSetLayoutHandles handles;
};
