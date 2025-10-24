#include "VulkanDescriptorManager.h"
#include "../Utils/ErrorHelper.h"
#include <array>

VulkanDescriptorManager::VulkanDescriptorManager(const VulkanHandles& vulkanHandles, vector<VulkanDescriptor*>& vulkanDescriptors) :
	vk(vulkanHandles)
{
	handles.descriptors.insert(handles.descriptors.end(), vulkanDescriptors.begin(), vulkanDescriptors.end());

	CreateDescriptorPool();
	AllocateDescriptorSet();
}

VulkanDescriptorManager::~VulkanDescriptorManager()
{
	vkDestroyDescriptorPool(vk.device, handles.descriptorPool, nullptr);

	for (const auto& descriptor : handles.descriptors)
	{
		delete(descriptor);
	}

	
}

void VulkanDescriptorManager::CreateDescriptorPool()
{
	descriptorCountByType = countDescriptorByType();

	vector<VkDescriptorPoolSize> poolSizes;

	for (const auto&[descType, count] : descriptorCountByType)
	{
		VkDescriptorPoolSize poolSize{};
		poolSize.type = descType;
		poolSize.descriptorCount = count;

		poolSizes.push_back(poolSize);
	}

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizes.size();
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = poolSizes.size();

	VK_CHECK(vkCreateDescriptorPool(vk.device, &poolInfo, nullptr, &handles.descriptorPool), "FAILED TO CREATE DESCRIPTOR POOL");
}

void VulkanDescriptorManager::AllocateDescriptorSet()
{
	for (const auto& descriptor : handles.descriptors)
	{
		descriptor->AllocateDescriptorSet(handles.descriptorPool);
	}
}

void VulkanDescriptorManager::WriteDescriptorSet()
{
	for (const auto& descriptor : handles.descriptors)
	{
		descriptor->WriteDescriptorSet();
	}
}

std::unordered_map<VkDescriptorType, glm::uint32_t> VulkanDescriptorManager::countDescriptorByType()
{
	unordered_map<VkDescriptorType, uint32_t> countAns;

	for (const auto& descriptor : handles.descriptors)
	{
		for (const auto& [descType, count] : descriptor->getHandles().descriptorCountByType)
		{
			countAns[descType] += count;
		}
	}

	return countAns;
}

