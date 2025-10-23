#pragma once
#include <vulkan/vulkan.h>
#include "VulkanContext.h"
#include <unordered_map>


using namespace std;

struct DescriptorSetLayoutHandles
{
	VkDescriptorSetLayout descSetLayout;
	
	unordered_map<VkDescriptorType, uint32_t> descriptorTypeCount;
};

class VulkanDescriptorSetLayout
{
public:
	VulkanDescriptorSetLayout(const VulkanHandles& vulkanHandles, const vector<VkDescriptorSetLayoutBinding>& layoutBindingList);
	~VulkanDescriptorSetLayout();

	const DescriptorSetLayoutHandles& getHandles() const { return handles; }

	const VkDescriptorSetLayout& getDescriptorSetLayout() const { return handles.descSetLayout; }

private:
	const VulkanHandles& vk;
	
	DescriptorSetLayoutHandles handles;

	void CountDescriptorType(const vector<VkDescriptorSetLayoutBinding>& layoutBindingList);
	void CreateDescSetLayout(const vector<VkDescriptorSetLayoutBinding>& layoutBindingList);
};
