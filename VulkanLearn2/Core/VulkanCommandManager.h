#pragma once
#include "VulkanContext.h"
#include <vector>

// Đề xuất đổi tên: CommandManagerData
struct CommandManagerHandles
{
	VkCommandPool commandPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> commandBuffers;
};

// Class quản lý việc tạo và sử dụng các command buffer.
// Bao gồm một command pool và các command buffer chính cho các frame-in-flight,
// cũng như các hàm tiện ích để tạo command buffer dùng một lần.
class VulkanCommandManager
{
public:
	VulkanCommandManager(const VulkanHandles& vulkanHandles, int MAX_FRAME_IN_FLIGHT);
	~VulkanCommandManager();
	
	// Lấy các handle nội bộ.
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
	void CreateCommandPool();
	void CreateCommandBuffers(int MAX_FRAME_IN_FLIGHT);
};