#include "pch.h"
#include "VulkanCommandManager.h"


VulkanCommandManager::VulkanCommandManager(const VulkanHandles& vulkanHandles, int MAX_FRAME_IN_FLIGHT):
	vk(vulkanHandles)
{
	CreateCommandPool();
	CreateCommandBuffer(MAX_FRAME_IN_FLIGHT);
}

VulkanCommandManager::~VulkanCommandManager()
{
	vkFreeCommandBuffers(vk.device, handles.commandPool, handles.commandBuffers.size(), handles.commandBuffers.data());
	vkDestroyCommandPool(vk.device, handles.commandPool, nullptr);
}

void VulkanCommandManager::CreateCommandPool()
{
	VkCommandPoolCreateInfo commandPoolInfo{};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.queueFamilyIndex = vk.queueFamilyIndices.GraphicQueueIndex;
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VK_CHECK(vkCreateCommandPool(vk.device, &commandPoolInfo, nullptr, &handles.commandPool), "FAILED TO CREATE COMMAND POOL");
}

void VulkanCommandManager::CreateCommandBuffer(int MAX_FRAMES_IN_FLIGHT)
{
	handles.commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = handles.commandPool;
	allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

	if (vkAllocateCommandBuffers(vk.device, &allocInfo, handles.commandBuffers.data()) != VK_SUCCESS)
	{
		throw runtime_error("FAILED TO ALLOC COMMAND BUFFER");
	}
}

VkCommandBuffer& VulkanCommandManager::BeginSingleTimeCmdBuffer()
{
	VkCommandBuffer cmdBuffer;
	VkCommandBufferAllocateInfo allocInfo{};

	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = handles.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VK_CHECK(vkAllocateCommandBuffers(vk.device, &allocInfo, &cmdBuffer), "FAILED TO ALLOCATE SINGLE TIME COMMAND BUFFER");

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = VK_NULL_HANDLE;

	vkBeginCommandBuffer(cmdBuffer, &beginInfo);

	return cmdBuffer;
}

void VulkanCommandManager::EndSingleTimeCmdBuffer(VkCommandBuffer cmdBuffer)
{
	vkEndCommandBuffer(cmdBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	vkQueueSubmit(vk.graphicQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkDeviceWaitIdle(vk.device);

	vkFreeCommandBuffers(vk.device, handles.commandPool, 1, &cmdBuffer);
}