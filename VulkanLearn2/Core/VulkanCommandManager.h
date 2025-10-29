#pragma once
#include "VulkanContext.h"

struct CommandManagerHandles
{
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;
};

class VulkanCommandManager
{
public:
	VulkanCommandManager(const VulkanHandles& vulkanHandles, int MAX_FRAME_IN_FLIGHT);
	~VulkanCommandManager();
	
	const CommandManagerHandles& getHandles() const { return handles; }

	VkCommandBuffer BeginSingleTimeCmdBuffer();
	void EndSingleTimeCmdBuffer(VkCommandBuffer cmdBuffer);
private:
	const VulkanHandles& vk;
	CommandManagerHandles handles;

	void CreateCommandPool();
	void CreateCommandBuffer(int MAX_FRAME_IN_FLIGHT);
};
