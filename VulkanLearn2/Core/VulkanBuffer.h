#pragma once
#include "VulkanContext.h"

// Forward declarations
class VulkanCommandManager;

// Struct chứa các handle và thông tin của một buffer Vulkan.
struct BufferHandles
{
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
	VkDeviceSize bufferSize = 0;
};

// Class đóng gói một buffer của Vulkan (VkBuffer).
// Có thể được dùng làm vertex buffer, index buffer, uniform buffer, staging buffer, v.v.
// Hỗ trợ hai chế độ upload dữ liệu:
// 1. Direct: Map bộ nhớ và ghi trực tiếp (dành cho buffer host-visible).
// 2. Staging: Dùng một buffer trung gian để copy dữ liệu sang buffer device-local (tối ưu hiệu năng).
class VulkanBuffer
{
public:
	// isUseStagingBuffer = false: Tạo buffer trên bộ nhớ CPU có thể thấy (host-visible), upload trực tiếp.
	// isUseStagingBuffer = true: Tạo buffer trên bộ nhớ GPU (device-local), upload qua staging buffer.
	VulkanBuffer(const VulkanHandles& vulkanHandles, VulkanCommandManager* const vulkanCommandManager, VkBufferCreateInfo& bufferInfo, bool isUseStagingBuffer = false);
	~VulkanBuffer();

	// Lấy các handle của buffer.
	const BufferHandles& getHandles() const { return m_Handles; }

	// Upload dữ liệu lên buffer. Tự động chọn phương thức upload (direct/staging) dựa trên cấu hình lúc tạo.
	void UploadData(const void* pSrcData, VkDeviceSize updateSize, VkDeviceSize offset);

private:
	// --- Tham chiếu Vulkan ---
	const VulkanHandles& m_VulkanHandles;
	VulkanCommandManager* const m_CommandManager;
	
	// --- Dữ liệu nội bộ ---
	BufferHandles m_Handles;
	bool m_IsUseStagingBuffer;

	// --- Hàm helper private ---

	// Hàm chung để tạo buffer và cấp phát bộ nhớ.
	void CreateBuffer(VkBufferCreateInfo bufferInfo, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory);

	// Upload dữ liệu bằng cách map bộ nhớ trực tiếp.
	void UploadDataViaDirect(const void* pSrcData, VkDeviceMemory& bufferMemory, VkDeviceSize updateSize, VkDeviceSize offset);
	// Upload dữ liệu bằng cách dùng một staging buffer trung gian.
	void UploadDataViaStagingBuffer(const void* pSrcData, VkDeviceSize updateSize, VkDeviceSize offset);

	// Tìm kiếm memory type index phù hợp.
	uint32_t findMemoryTypeIndex(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties);
};