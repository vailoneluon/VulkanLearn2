#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanContext.h"
#include "VulkanCommandManager.h"
#include "VulkanBuffer.h"
#include "VulkanTypes.h"

using namespace std;

struct DescriptorManagerHandles
{
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout perFrameDescriptorSetLayout;
	VkDescriptorSetLayout oneTimeDescriptorSetLayout;
	vector<VkDescriptorSet> perFrameDescriptorSets;
	VkDescriptorSet oneTimeDescriptorSet;
};

class VulkanDescriptorManager
{
public:
	VulkanDescriptorManager(const VulkanHandles& vulkanHandles, VulkanCommandManager* const cmd, const VkImageView& imageView, const VkSampler& sampler, int MAX_FRAMES_IN_FLIGHT);
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
};
