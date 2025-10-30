#include "pch.h"
#include "VulkanBuffer.h"
#include "VulkanCommandManager.h"

VulkanBuffer::VulkanBuffer(const VulkanHandles& vulkanHandles, VulkanCommandManager* const vulkanCommandManager, VkBufferCreateInfo& bufferInfo, bool isUseStagingBuffer):
	m_VulkanHandles(vulkanHandles), 
	m_CommandManager(vulkanCommandManager), 
	m_IsUseStagingBuffer(isUseStagingBuffer)
{
	m_Handles.bufferSize = bufferInfo.size;

	// Mặc định, buffer được tạo trên bộ nhớ device-local để có hiệu năng truy cập cao nhất từ GPU.
	VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	if (m_IsUseStagingBuffer)
	{
		// Nếu dùng staging, buffer chính (destination) cần có cờ USAGE_TRANSFER_DST để nhận dữ liệu.
		bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}
	else
	{
		// Nếu không dùng staging, buffer phải được tạo trên bộ nhớ host-visible để CPU có thể ghi trực tiếp.
		properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	}
	
	CreateBuffer(bufferInfo, properties, m_Handles.buffer, m_Handles.bufferMemory);
}

VulkanBuffer::~VulkanBuffer()
{
	// Giải phóng bộ nhớ và hủy buffer.
	vkFreeMemory(m_VulkanHandles.device, m_Handles.bufferMemory, nullptr);
	vkDestroyBuffer(m_VulkanHandles.device, m_Handles.buffer, nullptr);
}

void VulkanBuffer::CreateBuffer(VkBufferCreateInfo bufferInfo, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	// Tạo buffer handle.
	VK_CHECK(vkCreateBuffer(m_VulkanHandles.device, &bufferInfo, nullptr, &buffer), "LỖI: Tạo buffer thất bại!");

	// Lấy thông tin yêu cầu về bộ nhớ cho buffer.
	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(m_VulkanHandles.device, buffer, &memoryRequirements);

	// Chuẩn bị thông tin để cấp phát bộ nhớ.
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.memoryTypeIndex = findMemoryTypeIndex(memoryRequirements.memoryTypeBits, properties);
	allocInfo.allocationSize = memoryRequirements.size;

	// Cấp phát bộ nhớ.
	VK_CHECK(vkAllocateMemory(m_VulkanHandles.device, &allocInfo, nullptr, &bufferMemory), "LỖI: Cấp phát bộ nhớ cho buffer thất bại!");

	// Gắn bộ nhớ vào buffer.
	vkBindBufferMemory(m_VulkanHandles.device, buffer, bufferMemory, 0);
}


void VulkanBuffer::UploadData(const void* pSrcData, VkDeviceSize updateDataSize, VkDeviceSize offset)
{
	// Dựa vào cấu hình lúc khởi tạo để chọn phương thức upload phù hợp.
	if (m_IsUseStagingBuffer)
	{
		UploadDataViaStagingBuffer(pSrcData, updateDataSize, offset);
	}
	else
	{
		UploadDataViaDirect(pSrcData, m_Handles.bufferMemory, updateDataSize, offset);
	}
}

void VulkanBuffer::UploadDataViaDirect(const void* pSrcData, VkDeviceMemory& bufferMemory, VkDeviceSize updateDataSize, VkDeviceSize offset)
{
	// Ánh xạ (map) một vùng nhớ của buffer vào bộ nhớ của CPU.
	void* p;
	VK_CHECK(vkMapMemory(m_VulkanHandles.device, bufferMemory, 0, m_Handles.bufferSize, 0, &p), "LỖI: Map memory thất bại!");
	
	// Sao chép dữ liệu từ nguồn vào vùng nhớ đã map.
	memcpy(reinterpret_cast<char*>(p) + offset, pSrcData, updateDataSize);
	
	// Hủy ánh xạ (unmap).
	vkUnmapMemory(m_VulkanHandles.device, bufferMemory);
}

void VulkanBuffer::UploadDataViaStagingBuffer(const void* pSrcData, VkDeviceSize updateDataSize, VkDeviceSize offset)
{
	// 1. Tạo một staging buffer tạm thời trên bộ nhớ host-visible.
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.queueFamilyIndexCount = 1;
	bufferInfo.pQueueFamilyIndices = &m_VulkanHandles.queueFamilyIndices.GraphicQueueIndex;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.size = m_Handles.bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	CreateBuffer(bufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	// 2. Upload dữ liệu từ CPU vào staging buffer.
	UploadDataViaDirect(pSrcData, stagingBufferMemory, updateDataSize, offset);

	// 3. Dùng command buffer để copy dữ liệu từ staging buffer sang buffer chính (device-local).
	VkCommandBuffer singleTimeCmd = m_CommandManager->BeginSingleTimeCmdBuffer();

	VkBufferCopy region{};
	region.srcOffset = offset;
	region.dstOffset = offset;
	region.size = updateDataSize;

	vkCmdCopyBuffer(singleTimeCmd, stagingBuffer, m_Handles.buffer, 1, &region);

	m_CommandManager->EndSingleTimeCmdBuffer(singleTimeCmd);
	
	// 4. Giải phóng tài nguyên của staging buffer.
	vkFreeMemory(m_VulkanHandles.device, stagingBufferMemory, nullptr);
	vkDestroyBuffer(m_VulkanHandles.device, stagingBuffer, nullptr);
}

uint32_t VulkanBuffer::findMemoryTypeIndex(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(m_VulkanHandles.physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((memoryTypeBits & (1 << i)) &&
			(memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	throw std::runtime_error("LỖI: Không tìm thấy memory type phù hợp!");
}