#include "VulkanImage.h"
#include "../Utils/ErrorHelper.h"


VulkanImage::VulkanImage(const VulkanHandles& vulkanHandles, const VulkanImageCreateInfo& imageCreateInfo, const VulkanImageViewCreateInfo& imageViewCreateInfo) :
	vk(vulkanHandles)
{
	CreateImage(imageCreateInfo);
	CreateImageView(imageViewCreateInfo);
}

VulkanImage::~VulkanImage()
{
	vkDestroyImageView(vk.device, handles.imageView, nullptr);
	vkDestroyImage(vk.device, handles.image, nullptr);
	vkFreeMemory(vk.device, handles.imageMemory, nullptr);
}

void VulkanImage::CreateImage(const VulkanImageCreateInfo& vulkanImageInfo)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.queueFamilyIndexCount = 1;
	imageInfo.pQueueFamilyIndices = &vk.queueFamilyIndices.GraphicQueueIndex;
	imageInfo.extent.width = vulkanImageInfo.width;
	imageInfo.extent.height = vulkanImageInfo.height;
	imageInfo.extent.depth = 1;
	imageInfo.format = vulkanImageInfo.format;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.mipLevels = vulkanImageInfo.mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = vulkanImageInfo.samples;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = vulkanImageInfo.mipLevels > 1 ? vulkanImageInfo.imageUsageFlags | VK_IMAGE_USAGE_TRANSFER_SRC_BIT : vulkanImageInfo.imageUsageFlags;

	VK_CHECK(vkCreateImage(vk.device, &imageInfo, nullptr, &handles.image), "FAILED TO CREATE IMAGE");

	// Tạo memory
	VkMemoryRequirements imageMemoryRequirements{};
	vkGetImageMemoryRequirements(vk.device, handles.image, &imageMemoryRequirements);

	VkMemoryAllocateInfo imageAllocInfo{};
	imageAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	imageAllocInfo.allocationSize = imageMemoryRequirements.size;
	imageAllocInfo.memoryTypeIndex = findMemoryTypeIndex(imageMemoryRequirements.memoryTypeBits,
		vulkanImageInfo.memoryFlags);

	if (vkAllocateMemory(vk.device, &imageAllocInfo, nullptr, &handles.imageMemory) != VK_SUCCESS)
	{
		throw runtime_error("FAILED TO ALLOCATE TEXTURE IMAGE MEMORY");
	}

	vkBindImageMemory(vk.device, handles.image, handles.imageMemory, 0);
}

void VulkanImage::CreateImageView(const VulkanImageViewCreateInfo& vulkanImageViewInfo)
{
	VkImageViewCreateInfo imageViewInfo{};
	imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	
	imageViewInfo.image = handles.image;
	
	imageViewInfo.format = vulkanImageViewInfo.format;
	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	
	imageViewInfo.subresourceRange.aspectMask = vulkanImageViewInfo.aspectFlags;
	imageViewInfo.subresourceRange.levelCount = vulkanImageViewInfo.mipLevels;
	imageViewInfo.subresourceRange.layerCount = 1;
	imageViewInfo.subresourceRange.baseArrayLayer = 0;
	imageViewInfo.subresourceRange.baseMipLevel = 0;
	//imageViewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};

	VK_CHECK(vkCreateImageView(vk.device, &imageViewInfo, nullptr, &handles.imageView), "FAILED TO CREAT IMAGE VIEW");
}

uint32_t VulkanImage::findMemoryTypeIndex(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties)
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
