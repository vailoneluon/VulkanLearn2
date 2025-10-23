#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanContext.h"
#include "VulkanCommandManager.h"
#include "VulkanBuffer.h"
#include "VulkanTypes.h"
#include "VulkanDescriptorSetLayout.h"
#include <unordered_map>

using namespace std;

struct DescriptorManagerHandles
{
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout perFrameDescriptorSetLayout;
	VkDescriptorSetLayout oneTimeDescriptorSetLayout;
	vector<VkDescriptorSet> perFrameDescriptorSets;
	VkDescriptorSet oneTimeDescriptorSet;

	vector<VulkanDescriptorSetLayout*> descriptorSetLayouts;
};

class VulkanDescriptorManager
{
public:
	VulkanDescriptorManager(const VulkanHandles& vulkanHandles, VulkanCommandManager* const cmd, const VkImageView& imageView, const VkSampler& sampler, int MAX_FRAMES_IN_FLIGHT, vector<VulkanDescriptorSetLayout*>& setLayouts);
	~VulkanDescriptorManager();

	const DescriptorManagerHandles& getHandles() const { return handles; }

	void UpdateUniformBuffer(UniformBufferObject& ubo, int currentFrame);

private:
	const VulkanHandles& vk;
	VulkanCommandManager* const cmd;
	int MAX_FRAMES_IN_FLIGHT;

	vector<VulkanBuffer*> uboBuffers;

	DescriptorManagerHandles handles;

	void CreateUniformBuffer();

	void CreateDescriptorSetLayout();
	void CreateDescriptorPool();
	void CreateDescriptorSets(const VkImageView& imageView, const VkSampler& sampler);

	unordered_map<VkDescriptorType, uint32_t> countDescriptorByType(vector<VulkanDescriptorSetLayout*>& setLayouts);
};
