#pragma once
#include "VulkanContext.h"
#include <vector>


// Struct chứa các handle cho các đối tượng đồng bộ hóa.
struct SyncManagerHandles
{
	// Semaphores để báo hiệu rằng một image từ swapchain đã sẵn sàng để render.
	std::vector<VkSemaphore> imageAvailableSemaphores;
	// Semaphores để báo hiệu rằng việc render vào một image đã hoàn tất.
	std::vector<VkSemaphore> renderFinishedSemaphores;
	// Fences để đồng bộ hóa CPU-GPU, đảm bảo frame trước đã xong trước khi bắt đầu frame mới.
	std::vector<VkFence> inFlightFences;
};

// Class quản lý việc tạo và hủy các đối tượng đồng bộ hóa (semaphores và fences)
// cần thiết cho vòng lặp render.
class VulkanSyncManager
{
public:
	VulkanSyncManager(const VulkanHandles& vulkanHandles, int MAX_FRAMES_IN_FLIGHT, int swapchainImageCount);
	~VulkanSyncManager();

	// --- Getters ---
	const VkFence& getCurrentFence(int currentFrame) const;
	const VkSemaphore& getCurrentImageAvailableSemaphore(int currentFrame) const;
	const VkSemaphore& getCurrentRenderFinishedSemaphore(int imageIndex) const;
	
private:
	// --- Tham chiếu Vulkan ---
	const VulkanHandles& m_VulkanHandles;

	// --- Dữ liệu nội bộ ---
	SyncManagerHandles m_Handles;

	// --- Hàm helper private ---
	void CreateSyncObjects(int MAX_FRAMES_IN_FLIGHT, int swapchainImageCount);
};