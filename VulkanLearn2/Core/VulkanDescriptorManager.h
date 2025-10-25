#pragma once
#include "VulkanContext.h"
#include "VulkanDescriptor.h"


struct DescriptorManagerHandles
{
	VkDescriptorPool descriptorPool;

	vector<VulkanDescriptor*> descriptors;
};

class VulkanDescriptorManager
{
public:
	VulkanDescriptorManager(const VulkanHandles& vulkanHandles, vector<VulkanDescriptor*>& vulkanDescriptors);
	~VulkanDescriptorManager();

	const DescriptorManagerHandles& getHandles() const { return handles; }

private:
	const VulkanHandles& vk;

	DescriptorManagerHandles handles;

	unordered_map<VkDescriptorType, uint32_t> descriptorCountByType;

	void CreateDescriptorPool();
	void AllocateDescriptorSet();

	unordered_map<VkDescriptorType, uint32_t> countDescriptorByType();
};
