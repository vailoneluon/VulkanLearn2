#include "VulkanDescriptorManager.h"
#include "../Utils/ErrorHelper.h"
#include <array>

VulkanDescriptorManager::VulkanDescriptorManager(const VulkanHandles& vulkanHandles, VulkanCommandManager* const cmd, int MAX_FRAMES_IN_FLIGHT):
	vk(vulkanHandles), cmd(cmd), MAX_FRAMES_IN_FLIGHT(MAX_FRAMES_IN_FLIGHT)
{
	CreateUniformBuffer();

	CreateDescriptorSetLayout();
	CreateDescriptorPool();
	CreateDescriptorSets();
}

VulkanDescriptorManager::~VulkanDescriptorManager()
{
	for (auto buffer : uboBuffers)
	{
		delete(buffer);
	}

	vkDestroyDescriptorPool(vk.device, handles.descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(vk.device, handles.descriptorSetLayout, nullptr);
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
	VkDescriptorSetLayoutBinding uniformBindingInfo{};

	uniformBindingInfo.binding = 0;
	uniformBindingInfo.descriptorCount = 1;
	uniformBindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformBindingInfo.pImmutableSamplers = nullptr;
	uniformBindingInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	
	vector<VkDescriptorSetLayoutBinding> layoutBindings = { uniformBindingInfo };

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
	descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutInfo.bindingCount = layoutBindings.size();
	descriptorSetLayoutInfo.pBindings = layoutBindings.data();
	
	VK_CHECK(vkCreateDescriptorSetLayout(vk.device, &descriptorSetLayoutInfo, nullptr, &handles.descriptorSetLayout), "FAILED TO CREATE DESCRIPTOR SET LAYOUT");
}

void VulkanDescriptorManager::CreateDescriptorPool()
{
	array<VkDescriptorPoolSize, 1> descPoolSizes{};

	descPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descPoolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;

	VkDescriptorPoolCreateInfo descPoolInfo{};
	descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descPoolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
	descPoolInfo.poolSizeCount = descPoolSizes.size();
	descPoolInfo.pPoolSizes = descPoolSizes.data();
	
	VK_CHECK(vkCreateDescriptorPool(vk.device, &descPoolInfo, nullptr, &handles.descriptorPool), "FAILED TO CREATE DESCRIPTOR POOL");
}

void VulkanDescriptorManager::CreateDescriptorSets()
{
	vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, handles.descriptorSetLayout);

	VkDescriptorSetAllocateInfo descriptorSetInfo{};
	descriptorSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetInfo.descriptorPool = handles.descriptorPool;
	descriptorSetInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
	descriptorSetInfo.pSetLayouts = layouts.data();

	handles.descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	VK_CHECK(vkAllocateDescriptorSets(vk.device, &descriptorSetInfo, handles.descriptorSets.data()), "FAILED TO ALLOCATE DESCRIPTOR SET");

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
		descriptorWrite.dstSet = handles.descriptorSets[i];
		descriptorWrite.pBufferInfo = &bufferInfo;
		
		vkUpdateDescriptorSets(vk.device, 1, &descriptorWrite, 0, nullptr);
	}
	
}

