#include "VulkanCommandManager.h"
#include "../Utils/ErrorHelper.h"

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
	allocInfo.commandPool = handles.commandPool;
	allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	if (vkAllocateCommandBuffers(vk.device, &allocInfo, handles.commandBuffers.data()) != VK_SUCCESS)
	{
		throw runtime_error("FAILED TO ALLOC COMMAND BUFFER");
	}
}
