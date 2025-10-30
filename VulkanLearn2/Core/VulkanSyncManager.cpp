#include "pch.h"
#include "VulkanSyncManager.h"

VulkanSyncManager::VulkanSyncManager(const VulkanHandles& vulkanHandles, int MAX_FRAMES_IN_FLIGHT, int swapchainImageCount):
	m_VulkanHandles(vulkanHandles)
{
	CreateSyncObjects(MAX_FRAMES_IN_FLIGHT, swapchainImageCount);
}

VulkanSyncManager::~VulkanSyncManager()
{
	// Hủy tất cả các đối tượng đồng bộ hóa đã tạo.
	for (VkSemaphore s : m_Handles.imageAvailableSemaphores)
	{
		vkDestroySemaphore(m_VulkanHandles.device, s, nullptr);
	}

	for (VkSemaphore s : m_Handles.renderFinishedSemaphores)
	{
		vkDestroySemaphore(m_VulkanHandles.device, s, nullptr);
	}

	for (VkFence f : m_Handles.inFlightFences)
	{
		vkDestroyFence(m_VulkanHandles.device, f, nullptr);
	}
}

const VkFence& VulkanSyncManager::getCurrentFence(int currentFrame) const 
{
	return m_Handles.inFlightFences[currentFrame];
}

const VkSemaphore& VulkanSyncManager::getCurrentImageAvailableSemaphore(int currentFrame) const 
{
	return m_Handles.imageAvailableSemaphores[currentFrame];
}

const VkSemaphore& VulkanSyncManager::getCurrentRenderFinishedSemaphore(int imageIndex) const
{
	return m_Handles.renderFinishedSemaphores[imageIndex];
}

void VulkanSyncManager::CreateSyncObjects(int MAX_FRAMES_IN_FLIGHT, int swapchainImageCount)
{
	// Cấp phát bộ nhớ cho các vector chứa handle.
	m_Handles.inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	m_Handles.imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_Handles.renderFinishedSemaphores.resize(swapchainImageCount);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	// Tạo fence ở trạng thái đã được báo hiệu (signaled) để vòng lặp render đầu tiên không bị block.
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	// Tạo các đối tượng đồng bộ cho mỗi frame-in-flight.
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VK_CHECK(vkCreateFence(m_VulkanHandles.device, &fenceInfo, nullptr, &m_Handles.inFlightFences[i]), "LỖI: Tạo in-flight fence thất bại!");
		VK_CHECK(vkCreateSemaphore(m_VulkanHandles.device, &semaphoreInfo, nullptr, &m_Handles.imageAvailableSemaphores[i]), "LỖI: Tạo image available semaphore thất bại!");
	}

	// Tạo semaphore cho mỗi image trong swapchain.
	// LƯU Ý: Số lượng này có thể khác với MAX_FRAMES_IN_FLIGHT.
	for (int i = 0; i < swapchainImageCount; i++)
	{
		VK_CHECK(vkCreateSemaphore(m_VulkanHandles.device, &semaphoreInfo, nullptr, &m_Handles.renderFinishedSemaphores[i]), "LỖI: Tạo render finished semaphore thất bại!");
	}
}