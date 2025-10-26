#pragma once
#include "VulkanContext.h"

struct DescriptorHandles
{
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;

	uint32_t setIndex;
	std::unordered_map<VkDescriptorType, uint32_t> descriptorCountByType;
};


struct BindingElementInfo
{
	uint32_t              binding;

	VkDescriptorType      descriptorType;
	uint32_t              descriptorCount;
	VkShaderStageFlags    stageFlags;
	const VkSampler*      pImmutableSamplers = nullptr;

};

struct ImageBindingUpdateInfo
{
	uint32_t binding;
	VkDescriptorImageInfo imageInfo;
};

struct BufferBindingUpdateInfo
{
	uint32_t binding;
	VkDescriptorBufferInfo bufferInfo;
};


class VulkanDescriptor
{
public:
	VulkanDescriptor(const VulkanHandles& vulkanHandles, const std::vector<BindingElementInfo>& vulkanBindingInfos, uint32_t setIndex);
	~VulkanDescriptor();

	const DescriptorHandles& getHandles() const { return handles; }
	uint32_t getSetIndex() const { return handles.setIndex; }

	void AllocateDescriptorSet(const VkDescriptorPool& descriptorPool);

	void UpdateImageBinding(int updateCount, const ImageBindingUpdateInfo* pImageBindingInfo);
	void UpdateBufferBinding(int updateCount, const BufferBindingUpdateInfo* pBufferBindingInfo);

private:
	const VulkanHandles& vk;
	DescriptorHandles handles;

	const std::vector<BindingElementInfo> bindingInfos;
	BindingElementInfo getBindingElementInfo(uint32_t binding);

	void CreateSetLayout(const std::vector<BindingElementInfo>& bindingInfos);

	void CountDescriptorByType(const std::vector<BindingElementInfo>& bindingInfos);
};
