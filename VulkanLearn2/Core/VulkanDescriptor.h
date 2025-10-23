#pragma once
#include <vulkan/vulkan.h>
#include "VulkanContext.h"
#include <vector>
#include <unordered_map>

using namespace std;

struct DescriptorHandles
{
	VkDescriptorSetLayout descriptorSetLayout;

	vector<VkWriteDescriptorSet> writeDescriptorSetList;

	unordered_map<VkDescriptorType, uint32_t> descriptorCountByType;
};


struct BindingElementInfo
{
	uint32_t              binding;

	VkDescriptorType      descriptorType;
	uint32_t              descriptorCount;
	VkShaderStageFlags    stageFlags;
	const VkSampler*      pImmutableSamplers = nullptr;
	
	const VkDescriptorBufferInfo* bufferDataInfo = nullptr;
	const VkDescriptorImageInfo*  imageDataInfo = nullptr;
};



class VulkanDescriptor
{
public:
	VulkanDescriptor(const VulkanHandles& vulkanHandles, const vector<BindingElementInfo>& bindingInfos);
	~VulkanDescriptor();


private:
	const VulkanHandles& vk;
	DescriptorHandles handles;

	const DescriptorHandles& getHandles() const { return handles; }

	void CreateSetLayout(const vector<BindingElementInfo>& bindingInfos);
	void CreateWriteDescriptorSet(const vector<BindingElementInfo>& bindingInfos);
	void CountDescriptorByType(const vector<BindingElementInfo>& bindingInfos);
};
