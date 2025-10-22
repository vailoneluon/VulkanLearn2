#include "VulkanDescriptorManager.h"
#include "../Utils/ErrorHelper.h"
#include <array>

VulkanDescriptorManager::VulkanDescriptorManager(const VulkanHandles& vulkanHandles, VulkanCommandManager* const cmd,const VkImageView& imageView, const VkSampler& sampler, int MAX_FRAMES_IN_FLIGHT):
	vk(vulkanHandles), cmd(cmd), MAX_FRAMES_IN_FLIGHT(MAX_FRAMES_IN_FLIGHT)
{
	CreateUniformBuffer();

	CreateDescriptorSetLayout();
	CreateDescriptorPool();
	CreateDescriptorSets(imageView, sampler);
}

VulkanDescriptorManager::~VulkanDescriptorManager()
{
	for (auto buffer : uboBuffers)
	{
		delete(buffer);
	}

	vkDestroyDescriptorPool(vk.device, handles.descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(vk.device, handles.perFrameDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(vk.device, handles.oneTimeDescriptorSetLayout, nullptr);
}

void VulkanDescriptorManager::UpdateUniformBuffer(UniformBufferObject& ubo, int currentFrame)
{
	uboBuffers[currentFrame]->UploadData(&ubo, sizeof(UniformBufferObject), 0);
}

void VulkanDescriptorManager::CreateUniformBuffer()
{
	uboBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.queueFamilyIndexCount = 1;
	bufferInfo.pQueueFamilyIndices = &vk.queueFamilyIndices.GraphicQueueIndex;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.size = sizeof(UniformBufferObject);
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		uboBuffers[i] = new VulkanBuffer(vk, cmd, bufferInfo, false);
	}

}

void VulkanDescriptorManager::CreateDescriptorSetLayout()
{
	// Per Frame Layout
	VkDescriptorSetLayoutBinding uniformBindingInfo{};

	uniformBindingInfo.binding = 0;
	uniformBindingInfo.descriptorCount = 1;
	uniformBindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformBindingInfo.pImmutableSamplers = nullptr;
	uniformBindingInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	
	vector<VkDescriptorSetLayoutBinding> perFrameLayoutBindings = { uniformBindingInfo };

	VkDescriptorSetLayoutCreateInfo perFrameDescSetLayoutInfo{};
	perFrameDescSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	perFrameDescSetLayoutInfo.bindingCount = perFrameLayoutBindings.size();
	perFrameDescSetLayoutInfo.pBindings = perFrameLayoutBindings.data();
	
	VK_CHECK(vkCreateDescriptorSetLayout(vk.device, &perFrameDescSetLayoutInfo, nullptr, &handles.perFrameDescriptorSetLayout), "FAILED TO CREATE DESCRIPTOR SET LAYOUT");
	
	// One Time Layout
	VkDescriptorSetLayoutBinding sampledImageBindingInfo{};

	sampledImageBindingInfo.binding = 0;
	sampledImageBindingInfo.descriptorCount = 1;
	sampledImageBindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sampledImageBindingInfo.pImmutableSamplers = nullptr;
	sampledImageBindingInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	vector<VkDescriptorSetLayoutBinding> oneTimeLayoutBindings = { sampledImageBindingInfo };

	VkDescriptorSetLayoutCreateInfo oneTimeDescSetLayoutInfo{};
	oneTimeDescSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	oneTimeDescSetLayoutInfo.bindingCount = oneTimeLayoutBindings.size();
	oneTimeDescSetLayoutInfo.pBindings = oneTimeLayoutBindings.data();

	VK_CHECK(vkCreateDescriptorSetLayout(vk.device, &oneTimeDescSetLayoutInfo, nullptr, &handles.oneTimeDescriptorSetLayout), "FAILED TO CREATE DESCRIPTOR SET LAYOUT");

}

void VulkanDescriptorManager::CreateDescriptorPool()
{
	array<VkDescriptorPoolSize, 2> descPoolSizes{};

	descPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descPoolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;

	descPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descPoolSizes[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo descPoolInfo{};
	descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descPoolInfo.maxSets = MAX_FRAMES_IN_FLIGHT + 1;
	descPoolInfo.poolSizeCount = descPoolSizes.size();
	descPoolInfo.pPoolSizes = descPoolSizes.data();
	
	VK_CHECK(vkCreateDescriptorPool(vk.device, &descPoolInfo, nullptr, &handles.descriptorPool), "FAILED TO CREATE DESCRIPTOR POOL");
}

void VulkanDescriptorManager::CreateDescriptorSets(const VkImageView& imageView, const VkSampler& sampler)
{
	// Per Frame
	vector<VkDescriptorSetLayout> perFrameLayouts(MAX_FRAMES_IN_FLIGHT, handles.perFrameDescriptorSetLayout);

	VkDescriptorSetAllocateInfo perFrameDescriptorSetInfo{};
	perFrameDescriptorSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	perFrameDescriptorSetInfo.descriptorPool = handles.descriptorPool;
	perFrameDescriptorSetInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
	perFrameDescriptorSetInfo.pSetLayouts = perFrameLayouts.data();

	handles.perFrameDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	VK_CHECK(vkAllocateDescriptorSets(vk.device, &perFrameDescriptorSetInfo, handles.perFrameDescriptorSets.data()), "FAILED TO ALLOCATE DESCRIPTOR SET");

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uboBuffers[i]->getHandles().buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstBinding = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.dstSet = handles.perFrameDescriptorSets[i];
		descriptorWrite.pBufferInfo = &bufferInfo;
		
		vkUpdateDescriptorSets(vk.device, 1, &descriptorWrite, 0, nullptr);
	}
	

	// One Time
	VkDescriptorSetAllocateInfo oneTimeDescriptorSetInfo{};
	oneTimeDescriptorSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	oneTimeDescriptorSetInfo.descriptorPool = handles.descriptorPool;
	oneTimeDescriptorSetInfo.descriptorSetCount = 1;
	oneTimeDescriptorSetInfo.pSetLayouts = &handles.oneTimeDescriptorSetLayout;

	VK_CHECK(vkAllocateDescriptorSets(vk.device, &oneTimeDescriptorSetInfo, &handles.oneTimeDescriptorSet), "FAILED TO ALLOCATE DESCRIPTOR SET");

	// - write one time set
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = imageView;
	imageInfo.sampler = sampler;

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.dstSet = handles.oneTimeDescriptorSet;
	descriptorWrite.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(vk.device, 1, &descriptorWrite, 0, nullptr);
}

