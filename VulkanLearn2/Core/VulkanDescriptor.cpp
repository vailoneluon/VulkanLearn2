#include "VulkanDescriptor.h"
#include "../Utils/ErrorHelper.h"

VulkanDescriptor::VulkanDescriptor(const VulkanHandles& vulkanHandles, const vector<BindingElementInfo>& bindingInfos):
	vk(vulkanHandles)
{
	CreateSetLayout(bindingInfos);
	CreateWriteDescriptorSet(bindingInfos);
	CountDescriptorByType(bindingInfos);
}

VulkanDescriptor::~VulkanDescriptor()
{
	vkDestroyDescriptorSetLayout(vk.device, handles.descriptorSetLayout, nullptr);
}

void VulkanDescriptor::CreateSetLayout(const vector<BindingElementInfo>& bindingInfos)
{
	vector<VkDescriptorSetLayoutBinding> layoutBindings;
	for (const auto& bindingInfo : bindingInfos)
	{
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = bindingInfo.binding;
		layoutBinding.pImmutableSamplers = bindingInfo.pImmutableSamplers;
		layoutBinding.descriptorCount = bindingInfo.descriptorCount;
		layoutBinding.descriptorType = bindingInfo.descriptorType;
		layoutBinding.stageFlags = bindingInfo.stageFlags;

		layoutBindings.push_back(layoutBinding);
	}
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = layoutBindings.size();
		layoutInfo.pBindings = layoutBindings.data();

		VK_CHECK(vkCreateDescriptorSetLayout(vk.device, &layoutInfo, nullptr, &handles.descriptorSetLayout), "FAILED TO CREATE DESCRIPTOR SET LAYOUT");
}

void VulkanDescriptor::CreateWriteDescriptorSet(const vector<BindingElementInfo>& bindingInfos)
{
	for (const auto& bindingInfo : bindingInfos)
	{
		VkWriteDescriptorSet descriptorSetWrite{};
		descriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorSetWrite.dstBinding = bindingInfo.binding;
		descriptorSetWrite.descriptorType = bindingInfo.descriptorType;
		descriptorSetWrite.descriptorCount = bindingInfo.descriptorCount;
		descriptorSetWrite.dstArrayElement = 0;

		descriptorSetWrite.pBufferInfo = bindingInfo.bufferDataInfo;
		descriptorSetWrite.pImageInfo = bindingInfo.imageDataInfo;

		descriptorSetWrite.dstSet = VK_NULL_HANDLE;

		handles.writeDescriptorSetList.push_back(descriptorSetWrite);
	}
}

void VulkanDescriptor::CountDescriptorByType(const vector<BindingElementInfo>& bindingInfos)
{
	for (const auto& bindingInfo : bindingInfos)
	{
		handles.descriptorCountByType[bindingInfo.descriptorType] += bindingInfo.descriptorCount;
	}
}

