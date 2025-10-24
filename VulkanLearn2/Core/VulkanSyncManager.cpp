#include "pch.h"
#include "VulkanSyncManager.h"

VulkanSyncManager::VulkanSyncManager(const VulkanHandles& vulkanHandles, int MAX_FRAMES_IN_FLIGHT, int swapchainImageCount):
	vk(vulkanHandles)
{
	CreateSyncObject(MAX_FRAMES_IN_FLIGHT, swapchainImageCount);
}

VulkanSyncManager::~VulkanSyncManager()
{
	for (VkSemaphore s : handles.imageAvailableSemaphores)
	{
		vkDestroySemaphore(vk.device, s, nullptr);
	}

	for (VkSemaphore s : handles.renderFinishedSemaphores)
	{
		vkDestroySemaphore(vk.device, s, nullptr);
	}

	for (VkFence f : handles.inFlightFences)
	{
		vkDestroyFence(vk.device, f, nullptr);
	}
}

const VkFence& VulkanSyncManager::getCurrentFence(int currentFrame) const 
{
	return handles.inFlightFences[currentFrame];
}

const VkSemaphore& VulkanSyncManager::getCurrentImageAvailableSemaphore(int currentFrame) const 
{
	return handles.imageAvailableSemaphores[currentFrame];
}

const VkSemaphore& VulkanSyncManager::getCurrentRenderFinishedSemaphore(int imageIndex) const
{
	return handles.renderFinishedSemaphores[imageIndex];
}

void VulkanSyncManager::CreateSyncObject(int MAX_FRAMES_IN_FLIGHT, int swapchainImageCount)
{
	handles.inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	handles.imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	handles.renderFinishedSemaphores.resize(swapchainImageCount);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VK_CHECK(vkCreateFence(vk.device, &fenceInfo, nullptr, &handles.inFlightFences[i]), "FAILED TO CREATE IN FLIGHT FENCE");

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VK_CHECK(vkCreateSemaphore(vk.device, &semaphoreInfo, nullptr, &handles.imageAvailableSemaphores[i]), "FAILED TO CREATE IMAGE AVAILABLE SEMAPHORES");
	}

	for (int i = 0; i < swapchainImageCount; i++)
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VK_CHECK(vkCreateSemaphore(vk.device, &semaphoreInfo, nullptr, &handles.renderFinishedSemaphores[i]), "FAILED TO CREATE RENDER FININSH SEMAPHORES");
	}
}
