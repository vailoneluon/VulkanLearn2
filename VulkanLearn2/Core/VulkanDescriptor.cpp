#include "pch.h"
#include "VulkanDescriptor.h"


VulkanDescriptor::VulkanDescriptor(const VulkanHandles& vulkanHandles, const vector<BindingElementInfo>& vulkanBindingInfos, uint32_t setIndex):
	vk(vulkanHandles), bindingInfos(vulkanBindingInfos)
{
	handles.setIndex = setIndex;

	CreateSetLayout(vulkanBindingInfos);
	CountDescriptorByType(vulkanBindingInfos);
}

VulkanDescriptor::~VulkanDescriptor()
{
	// Chỉ thực hiện Delete ở trong VulkanDescriptorManager
	// Không thực hiện delete trong Main.
	vkDestroyDescriptorSetLayout(vk.device, handles.descriptorSetLayout, nullptr);
}

void VulkanDescriptor::CreateSetLayout(const vector<BindingElementInfo>& vulkanBindingInfos)
{
	vector<VkDescriptorSetLayoutBinding> layoutBindings;
	for (const auto& bindingInfo : vulkanBindingInfos)
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

void VulkanDescriptor::AllocateDescriptorSet(const VkDescriptorPool& descriptorPool)
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &handles.descriptorSetLayout;

	VK_CHECK(vkAllocateDescriptorSets(vk.device, &allocInfo, &handles.descriptorSet), "FAILED TO ALLOCATE DESCRIPTOR SET");
}

void VulkanDescriptor::WriteDescriptorSet()
{
	for (const auto& bindingInfo : bindingInfos)
	{
		VkWriteDescriptorSet descriptorSetWrite{};
		descriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorSetWrite.dstBinding = bindingInfo.binding;
		descriptorSetWrite.descriptorType = bindingInfo.descriptorType;
		descriptorSetWrite.descriptorCount = bindingInfo.descriptorCount;
		descriptorSetWrite.dstArrayElement = 0;

		descriptorSetWrite.pBufferInfo = &bindingInfo.bufferDataInfo;
		descriptorSetWrite.pImageInfo = &bindingInfo.imageDataInfo;

		descriptorSetWrite.dstSet = handles.descriptorSet;

		vkUpdateDescriptorSets(vk.device, 1, &descriptorSetWrite, 0, nullptr);
	}
}

void VulkanDescriptor::CountDescriptorByType(const vector<BindingElementInfo>& bindingInfos)
{
	for (const auto& bindingInfo : bindingInfos)
	{
		handles.descriptorCountByType[bindingInfo.descriptorType] += bindingInfo.descriptorCount;
	}
}

