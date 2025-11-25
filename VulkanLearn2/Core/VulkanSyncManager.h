#pragma once
#include "VulkanContext.h"
#include <vector>


// =================================================================================================
// Struct: SyncManagerHandles
// Mô tả: Chứa các handle cho các đối tượng đồng bộ hóa.
//        Bao gồm Semaphores và Fences.
// =================================================================================================
struct SyncManagerHandles
{
	// Semaphores để báo hiệu rằng một image từ swapchain đã sẵn sàng để render.
	std::vector<VkSemaphore> imageAvailableSemaphores;
	// Semaphores để báo hiệu rằng việc render vào một image đã hoàn tất.
	std::vector<VkSemaphore> renderFinishedSemaphores;
	// Fences để đồng bộ hóa CPU-GPU, đảm bảo frame trước đã xong trước khi bắt đầu frame mới.
	std::vector<VkFence> inFlightFences;
};

// =================================================================================================
// Class: VulkanSyncManager
// Mô tả: 
//      Quản lý việc tạo và hủy các đối tượng đồng bộ hóa (semaphores và fences)
//      cần thiết cho vòng lặp render.
// =================================================================================================
class VulkanSyncManager
{
public:
	// Constructor: Khởi tạo các đối tượng đồng bộ hóa.
	// Tham số:
	//      vulkanHandles: Tham chiếu đến các handle Vulkan chung.
	//      MAX_FRAMES_IN_FLIGHT: Số lượng frame được xử lý song song tối đa.
	//      swapchainImageCount: Số lượng image trong swapchain.
	VulkanSyncManager(const VulkanHandles& vulkanHandles, int MAX_FRAMES_IN_FLIGHT, int swapchainImageCount);
	~VulkanSyncManager();

	// --- Getters ---
	
	// Lấy fence cho frame hiện tại.
	const VkFence& getCurrentFence(int currentFrame) const;
	
	// Lấy semaphore báo hiệu image sẵn sàng cho frame hiện tại.
	const VkSemaphore& getCurrentImageAvailableSemaphore(int currentFrame) const;
	
	// Lấy semaphore báo hiệu render hoàn tất cho image index tương ứng.
	const VkSemaphore& getCurrentRenderFinishedSemaphore(int imageIndex) const;
	
private:
	// --- Tham chiếu Vulkan ---
	const VulkanHandles& m_VulkanHandles;

	// --- Dữ liệu nội bộ ---
	SyncManagerHandles m_Handles;

	// --- Hàm helper private ---
	
	// Helper: Tạo các đối tượng đồng bộ hóa.
	void CreateSyncObjects(int MAX_FRAMES_IN_FLIGHT, int swapchainImageCount);
};