#pragma once
#include "VulkanContext.h"

struct SyncManagerHandles
{
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
};

class VulkanSyncManager
{
public:
	VulkanSyncManager(const VulkanHandles& vulkanHandles, int MAX_FRAMES_IN_FLIGHT, int swapchainImageCount);
	~VulkanSyncManager();

	const VkFence& getCurrentFence(int currentFrame) const;
	const VkSemaphore& getCurrentImageAvailableSemaphore(int currentFrame) const ;
	const VkSemaphore& getCurrentRenderFinishedSemaphore(int imageIndex) const ;
	
private:
	const VulkanHandles& vk;
	SyncManagerHandles handles;

	void CreateSyncObject(int MAX_FRAMES_IN_FLIGHT, int swapchainImageCount);
};
