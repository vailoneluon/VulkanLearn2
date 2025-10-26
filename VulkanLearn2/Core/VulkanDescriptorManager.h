#pragma once
#include "VulkanContext.h"
#include "VulkanDescriptor.h"


struct DescriptorManagerHandles
{
	VkDescriptorPool descriptorPool;

	std::vector<VulkanDescriptor*> descriptors;
};

class VulkanDescriptorManager
{
public:
	VulkanDescriptorManager(const VulkanHandles& vulkanHandles, std::vector<VulkanDescriptor*>& vulkanDescriptors);
	~VulkanDescriptorManager();

	const DescriptorManagerHandles& getHandles() const { return handles; }

private:
	const VulkanHandles& vk;

	DescriptorManagerHandles handles;

	std::unordered_map<VkDescriptorType, uint32_t> descriptorCountByType;

	void CreateDescriptorPool();
	void AllocateDescriptorSet();

	std::unordered_map<VkDescriptorType, uint32_t> countDescriptorByType();
};
