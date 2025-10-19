#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanContext.h"
using namespace std;

struct SyncManagerHandles
{
	vector<VkSemaphore> imageAvailableSemaphores;
	vector<VkSemaphore> renderFinishedSemaphores;
	vector<VkFence> inFlightFences;
};

class VulkanSyncManager
{
public:
	VulkanSyncManager(const VulkanHandles& vulkanHandles, int MAX_FRAMES_IN_FLIGHT, int swapchainImageCount);
	~VulkanSyncManager();
	
private:
	const VulkanHandles& vk;
	SyncManagerHandles handles;

	void CreateSyncObject(int MAX_FRAMES_IN_FLIGHT, int swapchainImageCount);
};
