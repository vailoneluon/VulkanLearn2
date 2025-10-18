#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanContext.h"
using namespace std;

struct CommandManagerHandles
{
	VkCommandPool commandPool;
	vector<VkCommandBuffer> commandBuffers;
};

class VulkanCommandManager
{
public:
	VulkanCommandManager(const VulkanHandles& vulkanHandles, int MAX_FRAME_IN_FLIGHT);
	~VulkanCommandManager();
	
	const CommandManagerHandles& getHandles() const { return handles; }

private:
	const VulkanHandles& vk;
	CommandManagerHandles handles;

	void CreateCommandPool();
	void CreateCommandBuffer(int MAX_FRAME_IN_FLIGHT);
};
