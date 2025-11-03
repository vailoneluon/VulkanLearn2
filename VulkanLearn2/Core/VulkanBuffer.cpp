#include "pch.h"
#include "VulkanBuffer.h"
#include "VulkanCommandManager.h"

VulkanBuffer::VulkanBuffer(const VulkanHandles& vulkanHandles, VulkanCommandManager* const vulkanCommandManager, VkBufferCreateInfo& bufferInfo, VmaMemoryUsage memoryUsage)
	: m_VulkanHandles(vulkanHandles)
	, m_CommandManager(vulkanCommandManager)
	, m_MemoryUsage(memoryUsage)
{
	// Lưu lại kích thước buffer để tham khảo sau này.
	m_Handles.bufferSize = bufferInfo.size;

	// Nếu buffer được sử dụng trên GPU và cần cập nhật dữ liệu từ CPU,
	// nó phải có cờ USAGE_TRANSFER_DST_BIT để có thể làm đích đến cho một lệnh sao chép.
	if (m_MemoryUsage == VMA_MEMORY_USAGE_GPU_ONLY)
	{
		bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = m_MemoryUsage;

	// Đối với các buffer cần ghi dữ liệu thường xuyên từ CPU (CPU_TO_GPU hoặc CPU_ONLY),
	// chúng ta yêu cầu VMA map sẵn vùng nhớ này vĩnh viễn bằng cờ VMA_ALLOCATION_CREATE_MAPPED_BIT.
	// Điều này giúp tránh chi phí map/unmap mỗi lần cập nhật.
	if (m_MemoryUsage == VMA_MEMORY_USAGE_CPU_TO_GPU || m_MemoryUsage == VMA_MEMORY_USAGE_CPU_ONLY)
	{
		allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}

	// Thực hiện tạo buffer và cấp phát bộ nhớ bằng VMA.
	VmaAllocationInfo allocationResultInfo;
	VK_CHECK(vmaCreateBuffer(
		m_VulkanHandles.allocator,
		&bufferInfo, // Thông tin tạo buffer
		&allocInfo,  // Thông tin cấp phát bộ nhớ
		&m_Handles.buffer, // Handle buffer trả về
		&m_Handles.allocation, // Handle cấp phát trả về
		&allocationResultInfo // Thông tin chi tiết về việc cấp phát
	), "Lỗi: Không thể tạo buffer!");

	// Nếu VMA đã map sẵn vùng nhớ, lưu lại con trỏ pMappedData.
	if (allocationResultInfo.pMappedData)
	{
		m_Handles.pMappedData = allocationResultInfo.pMappedData;
	}
}

VulkanBuffer::~VulkanBuffer()
{
	// Hủy buffer và giải phóng bộ nhớ đã được cấp phát bởi VMA.
	vmaDestroyBuffer(m_VulkanHandles.allocator, m_Handles.buffer, m_Handles.allocation);
}

void VulkanBuffer::UploadData(const void* pSrcData, VkDeviceSize updateDataSize, VkDeviceSize offset)
{
	// Kịch bản 1: Ghi dữ liệu trực tiếp (CPU_ONLY hoặc CPU_TO_GPU)
	// Nếu buffer có vùng nhớ được map sẵn, chúng ta chỉ cần dùng memcpy để sao chép dữ liệu.
	if (m_MemoryUsage == VMA_MEMORY_USAGE_CPU_ONLY || m_MemoryUsage == VMA_MEMORY_USAGE_CPU_TO_GPU)
	{
		if (m_Handles.pMappedData == nullptr)
		{
			showError("Lỗi: Buffer được tạo để ghi trực tiếp nhưng không tìm thấy con trỏ mapped data.");
			return;
		}

		// Sao chép dữ liệu vào vùng nhớ đã map, có tính đến offset.
		char* pDest = reinterpret_cast<char*>(m_Handles.pMappedData) + offset;
		memcpy(pDest, pSrcData, updateDataSize);
	}
	// Kịch bản 2: Tải dữ liệu lên GPU thông qua staging buffer (GPU_ONLY)
	// Đây là cách hiệu quả để chuyển dữ liệu từ CPU sang bộ nhớ chỉ dành cho GPU.
	else if (m_MemoryUsage == VMA_MEMORY_USAGE_GPU_ONLY)
	{
		// 1. Tạo một staging buffer tạm thời trên CPU.
		// Buffer này có thể được map và ghi dữ liệu trực tiếp.
		VkBuffer stagingBuffer;
		VmaAllocation stagingAllocation;

		VkBufferCreateInfo stagingBufferInfo{};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.size = updateDataSize;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; // Dùng làm nguồn cho lệnh sao chép
		stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo stagingAllocInfo{};
		stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY; // Chỉ cần trên CPU
		stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT; // Map sẵn để ghi

		VmaAllocationInfo allocResultInfo;
		VK_CHECK(vmaCreateBuffer(m_VulkanHandles.allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation, &allocResultInfo),
			"Lỗi: Không thể tạo staging buffer!");

		// 2. Sao chép dữ liệu từ nguồn vào staging buffer.
		memcpy(allocResultInfo.pMappedData, pSrcData, updateDataSize);

		// 3. Ghi lệnh sao chép từ staging buffer sang buffer đích (trên GPU).
		VkCommandBuffer cmd = m_CommandManager->BeginSingleTimeCmdBuffer();

		VkBufferCopy region{};
		region.srcOffset = 0; // Bắt đầu từ đầu staging buffer
		region.dstOffset = offset; // Ghi vào vị trí offset của buffer đích
		region.size = updateDataSize; // Sao chép toàn bộ dữ liệu

		vkCmdCopyBuffer(cmd, stagingBuffer, m_Handles.buffer, 1, &region);

		m_CommandManager->EndSingleTimeCmdBuffer(cmd);

		// 4. Hủy staging buffer tạm thời ngay sau khi sao chép xong.
		vmaDestroyBuffer(m_VulkanHandles.allocator, stagingBuffer, stagingAllocation);
	}
}