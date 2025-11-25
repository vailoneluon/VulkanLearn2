#pragma once
#include "VulkanContext.h"
#include <vector>

// =================================================================================================
// Struct: CommandManagerHandles
// Mô tả: Chứa các handle nội bộ của VulkanCommandManager.
//        Bao gồm VkCommandPool và danh sách các VkCommandBuffer.
// =================================================================================================
struct CommandManagerHandles
{
	VkCommandPool commandPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> commandBuffers;
};

// =================================================================================================
// Class: VulkanCommandManager
// Mô tả: 
//      Quản lý việc tạo và sử dụng các command buffer.
//      Bao gồm một command pool và các command buffer chính cho các frame-in-flight,
//      cũng như các hàm tiện ích để tạo command buffer dùng một lần.
// =================================================================================================
class VulkanCommandManager
{
public:
	// Constructor: Khởi tạo command pool và cấp phát các command buffer chính.
	// Tham số:
	//      vulkanHandles: Tham chiếu đến các handle Vulkan chung.
	//      MAX_FRAME_IN_FLIGHT: Số lượng frame được xử lý song song tối đa.
	VulkanCommandManager(const VulkanHandles& vulkanHandles, int MAX_FRAME_IN_FLIGHT);
	~VulkanCommandManager();
	
	// Getter: Lấy các handle nội bộ.
	const CommandManagerHandles& getHandles() const { return m_Handles; }

	// Bắt đầu một command buffer để thực hiện các tác vụ chỉ diễn ra một lần (ví dụ: copy buffer).
	VkCommandBuffer BeginSingleTimeCmdBuffer();
	
	// Kết thúc, submit, và giải phóng command buffer dùng một lần.
	void EndSingleTimeCmdBuffer(VkCommandBuffer cmdBuffer);

private:
	// --- Tham chiếu Vulkan ---
	const VulkanHandles& m_VulkanHandles;

	// --- Dữ liệu nội bộ ---
	CommandManagerHandles m_Handles;

	// --- Hàm helper private ---
	
	// Helper: Tạo command pool.
	void CreateCommandPool();
	
	// Helper: Cấp phát các command buffer chính.
	void CreateCommandBuffers(int MAX_FRAME_IN_FLIGHT);
};