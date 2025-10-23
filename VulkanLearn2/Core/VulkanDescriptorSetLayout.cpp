#include "VulkanDescriptorSetLayout.h"
#include "../Utils/ErrorHelper.h"

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(const VulkanHandles& vulkanHandles, const vector<VkDescriptorSetLayoutBinding>& layoutBindingList):
	vk(vulkanHandles)
{
	CountDescriptorType(layoutBindingList);
	CreateDescSetLayout(layoutBindingList);
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
{
	vkDestroyDescriptorSetLayout(vk.device, handles.descSetLayout, nullptr);
}

void VulkanDescriptorSetLayout::CountDescriptorType(const vector<VkDescriptorSetLayoutBinding>& layoutBindingList)
{
	for (const auto& layoutBinding : layoutBindingList)
	{
		handles.descriptorTypeCount[layoutBinding.descriptorType] += layoutBinding.descriptorCount;
	}
}

void VulkanDescriptorSetLayout::CreateDescSetLayout(const vector<VkDescriptorSetLayoutBinding>& layoutBindingList)
{
	VkDescriptorSetLayoutCreateInfo descSetLayoutInfo{};
	descSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetLayoutInfo.bindingCount = layoutBindingList.size();
	descSetLayoutInfo.pBindings = layoutBindingList.data();

	VK_CHECK(vkCreateDescriptorSetLayout(vk.device, &descSetLayoutInfo, nullptr, &handles.descSetLayout), "FAILED TO CREATE DESCRIPTOR SET LAYOUT");
}
