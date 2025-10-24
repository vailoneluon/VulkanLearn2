#pragma once
#include "VulkanContext.h"

struct DescriptorHandles
{
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;

	unordered_map<VkDescriptorType, uint32_t> descriptorCountByType;
};


struct BindingElementInfo
{
	uint32_t              binding;

	VkDescriptorType      descriptorType;
	uint32_t              descriptorCount;
	VkShaderStageFlags    stageFlags;
	const VkSampler*      pImmutableSamplers = nullptr;
	
	VkDescriptorBufferInfo bufferDataInfo;
	VkDescriptorImageInfo  imageDataInfo;
};


class VulkanDescriptor
{
public:
	VulkanDescriptor(const VulkanHandles& vulkanHandles, const vector<BindingElementInfo>& vulkanBindingInfos);
	~VulkanDescriptor();

	const DescriptorHandles& getHandles() const { return handles; }

	void AllocateDescriptorSet(const VkDescriptorPool& descriptorPool);
	void WriteDescriptorSet();

private:
	const VulkanHandles& vk;
	DescriptorHandles handles;

	const vector<BindingElementInfo> bindingInfos;

	void CreateSetLayout(const vector<BindingElementInfo>& bindingInfos);

	void CountDescriptorByType(const vector<BindingElementInfo>& bindingInfos);
};
