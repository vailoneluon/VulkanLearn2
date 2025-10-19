#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanContext.h"
#include "VulkanCommandManager.h"

using namespace std;

struct BufferHandles
{
	VkBuffer buffer;
	VkDeviceMemory bufferMemory;
};

class VulkanBuffer
{
public:
	VulkanBuffer(const VulkanHandles& vulkanHandles, VulkanCommandManager* const vulkanCommandManager, VkBufferCreateInfo& bufferInfo, bool isUseStagingBuffer = false);
	~VulkanBuffer();

	const BufferHandles& getHandles() const { return handles; }

	void UploadData(const void* pSrcData, VkDeviceSize updateSize, VkDeviceSize offset);

private:
	const VulkanHandles& vk;
	VulkanCommandManager* const cmd;
	BufferHandles handles;

	bool isUseStagingBuffer;
	VkDeviceSize bufferSize;

	void CreateBuffer(VkBufferCreateInfo bufferInfo, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory);

	void UploadDataViaDirect(const void* pSrcData, VkDeviceMemory& bufferMemory, VkDeviceSize updateSize, VkDeviceSize offset);
	void UploadDataViaStagingBuffer(const void* pSrcData, VkDeviceSize updateSize, VkDeviceSize offset);

	uint32_t findMemoryTypeIndex(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties);
};
