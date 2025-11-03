#pragma once
#include "VulkanContext.h"

// Forward declarations
class VulkanCommandManager;

/**
 * @struct BufferHandles
 * @brief Chứa các handle và thông tin cần thiết của một Vulkan Buffer.
 *
 * Struct này đóng gói các đối tượng Vulkan cốt lõi liên quan đến một buffer,
 * giúp việc quản lý và truy cập chúng dễ dàng hơn.
 */
struct BufferHandles
{
	VkBuffer buffer = VK_NULL_HANDLE;			///< Handle của VkBuffer.
	VmaAllocation allocation = VK_NULL_HANDLE;	///< Handle cấp phát bộ nhớ từ VMA.
	VkDeviceSize bufferSize = 0;				///< Kích thước của buffer (tính bằng byte).
	void* pMappedData = nullptr;				///< Con trỏ tới vùng nhớ đã map (nếu có), cho phép ghi dữ liệu trực tiếp từ CPU.
};

/**
 * @class VulkanBuffer
 * @brief Đóng gói và quản lý một buffer trong Vulkan (VkBuffer) sử dụng thư viện VMA.
 *
 * Class này trừu tượng hóa việc tạo, hủy và cập nhật dữ liệu cho các loại buffer khác nhau
 * như vertex buffer, index buffer, uniform buffer, staging buffer, v.v.
 * Nó hỗ trợ hai kịch bản cập nhật dữ liệu chính:
 * 1. Ghi trực tiếp từ CPU (CPU_TO_GPU, CPU_ONLY): Dữ liệu được ghi vào một vùng nhớ được map vĩnh viễn.
 * 2. Tải lên GPU (GPU_ONLY): Dữ liệu được sao chép thông qua một staging buffer tạm thời.
 */
class VulkanBuffer
{
public:
	/**
	 * @brief Khởi tạo một VulkanBuffer.
	 * @param vulkanHandles Tham chiếu đến các handle Vulkan chung của ứng dụng.
	 * @param vulkanCommandManager Con trỏ tới CommandManager để thực hiện các lệnh sao chép.
	 * @param bufferInfo Struct chứa thông tin để tạo buffer (kích thước, cờ sử dụng, v.v.).
	 * @param memoryUsage Cách thức sử dụng bộ nhớ (ví dụ: chỉ GPU, CPU sang GPU).
	 */
	VulkanBuffer(const VulkanHandles& vulkanHandles, VulkanCommandManager* const vulkanCommandManager, VkBufferCreateInfo& bufferInfo, VmaMemoryUsage memoryUsage);

	/**
	 * @brief Hủy buffer và giải phóng bộ nhớ đã cấp phát.
	 */
	~VulkanBuffer();

	// Cấm sao chép và gán để tránh quản lý tài nguyên sai lầm.
	VulkanBuffer(const VulkanBuffer&) = delete;
	VulkanBuffer& operator=(const VulkanBuffer&) = delete;

	/**
	 * @brief Tải dữ liệu từ một con trỏ nguồn lên buffer.
	 *
	 * Phương thức này sẽ tự động chọn cách tải dữ liệu phù hợp (ghi trực tiếp hoặc dùng staging buffer)
	 * dựa trên `VmaMemoryUsage` đã được chỉ định khi tạo buffer.
	 *
	 * @param pSrcData Con trỏ tới dữ liệu nguồn cần tải lên.
	 * @param updateSize Kích thước của dữ liệu cần tải lên (tính bằng byte).
	 * @param offset Vị trí bắt đầu ghi dữ liệu trong buffer (tính bằng byte).
	 */
	void UploadData(const void* pSrcData, VkDeviceSize updateSize, VkDeviceSize offset = 0);

	/**
	 * @brief Lấy các handle và thông tin của buffer.
	 * @return Tham chiếu hằng tới struct BufferHandles.
	 */
	const BufferHandles& GetHandles() const { return m_Handles; }

private:
	// --- Dữ liệu nội bộ ---
	BufferHandles m_Handles;			///< Các handle và thông tin của buffer.
	VmaMemoryUsage m_MemoryUsage;		///< Cách thức sử dụng bộ nhớ đã định nghĩa.

	// --- Tham chiếu Vulkan ---
	const VulkanHandles& m_VulkanHandles;			///< Các handle Vulkan chung.
	VulkanCommandManager* const m_CommandManager;	///< Con trỏ tới CommandManager.
};
