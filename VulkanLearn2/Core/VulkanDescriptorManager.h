#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanContext.h"
#include "VulkanCommandManager.h"
#include "VulkanBuffer.h"
#include "VulkanTypes.h"
#include <unordered_map>
#include "VulkanDescriptor.h"

using namespace std;

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
	void WriteDescriptorSet();

	unordered_map<VkDescriptorType, uint32_t> countDescriptorByType();
};
