#include "VulkanBuffer.h"
#include "../Utils/ErrorHelper.h"

VulkanBuffer::VulkanBuffer(const VulkanHandles& vulkanHandles, VulkanCommandManager* const vulkanCommandManager, VkBufferCreateInfo& bufferInfo, bool isUseStagingBuffer):
	vk(vulkanHandles), cmd(vulkanCommandManager), isUseStagingBuffer(isUseStagingBuffer)
{
	handles.bufferSize = bufferInfo.size;

	VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	if (isUseStagingBuffer)
	{
		bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}
	else
	{
		properties |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	}
	
	CreateBuffer(bufferInfo, properties, handles.buffer, handles.bufferMemory);
}

VulkanBuffer::~VulkanBuffer()
{
	vkFreeMemory(vk.device, handles.bufferMemory, nullptr);
	vkDestroyBuffer(vk.device, handles.buffer, nullptr);
}

void VulkanBuffer::CreateBuffer(VkBufferCreateInfo bufferInfo, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VK_CHECK(vkCreateBuffer(vk.device, &bufferInfo, nullptr, &buffer), "FAILED TO CREATE BUFFER");

	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(vk.device, buffer, &memoryRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.memoryTypeIndex = findMemoryTypeIndex(memoryRequirements.memoryTypeBits, properties);
	allocInfo.allocationSize = memoryRequirements.size;

	VK_CHECK(vkAllocateMemory(vk.device, &allocInfo, nullptr, &bufferMemory), "FAILED TO ALLOCATE BUFFER MEMORY");

	vkBindBufferMemory(vk.device, buffer, bufferMemory, 0);
}


void VulkanBuffer::UploadData(const void* pSrcData, VkDeviceSize updateDataSize, VkDeviceSize offset)
{
	if (isUseStagingBuffer)
	{
		UploadDataViaStagingBuffer(pSrcData, updateDataSize, offset);
	}
	else
	{
		UploadDataViaDirect(pSrcData, handles.bufferMemory, updateDataSize, offset);
	}
}

void VulkanBuffer::UploadDataViaDirect(const void* pSrcData, VkDeviceMemory& bufferMemory, VkDeviceSize updateDataSize, VkDeviceSize offset)
{
	void* p;
	VK_CHECK(vkMapMemory(vk.device, bufferMemory, 0, handles.bufferSize, 0, &p), "FAILED TO MAP MEMORY");
	memcpy(reinterpret_cast<char*>(p) + offset, pSrcData, updateDataSize);
	vkUnmapMemory(vk.device, bufferMemory);
}

void VulkanBuffer::UploadDataViaStagingBuffer(const void* pSrcData, VkDeviceSize updateDataSize, VkDeviceSize offset)
{
	// Tạo Staging Buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.queueFamilyIndexCount = 1;
	bufferInfo.pQueueFamilyIndices = &vk.queueFamilyIndices.GraphicQueueIndex;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.size = handles.bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	CreateBuffer(bufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	// Upload vào Staging Buffer
	UploadDataViaDirect(pSrcData, stagingBufferMemory, updateDataSize, offset);

	// Copy Staging Buffer Sang Buffer
	VkCommandBuffer singleTimeCmd = cmd->BeginSingleTimeCmdBuffer();

	VkBufferCopy region{};
	region.srcOffset = offset;
	region.dstOffset = offset;
	region.size = updateDataSize;

	vkCmdCopyBuffer(singleTimeCmd, stagingBuffer, handles.buffer, 1, &region);

	cmd->EndSingleTimeCmdBuffer(singleTimeCmd);
	
	//Giải phóng tài nguyên
	vkFreeMemory(vk.device, stagingBufferMemory, nullptr);
	vkDestroyBuffer(vk.device, stagingBuffer, nullptr);
}

uint32_t VulkanBuffer::findMemoryTypeIndex(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(vk.physicalDevice, &memoryProperties);

	for (int i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((memoryTypeBits & (1 << i)) &&
			(memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	throw runtime_error("FAILED TO FIND SUITABLE MEMMORY TYPE");
}
