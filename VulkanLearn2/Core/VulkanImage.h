#pragma once
#include <vulkan/vulkan.h>
#include "VulkanContext.h"

struct ImageHandles
{
	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;
};

struct VulkanImageCreateInfo
{
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t mipLevels = 1;
	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
	VkFormat format = VK_FORMAT_UNDEFINED;
	VkImageUsageFlags imageUsageFlags = 0;
	VkMemoryPropertyFlags memoryFlags = 0;
};

struct VulkanImageViewCreateInfo
{
	VkFormat format = VK_FORMAT_UNDEFINED;
	VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_NONE;
	uint32_t mipLevels = 1;
};

class VulkanImage
{
public:
	VulkanImage(const VulkanHandles& vulkanHandles, const VulkanImageCreateInfo& imageCreateInfo, const VulkanImageViewCreateInfo& imageViewCreateInfo);
	~VulkanImage();

	const ImageHandles& getHandles() const{ return handles; }
private:
	ImageHandles handles;
	const VulkanHandles& vk;

	void CreateImage(const VulkanImageCreateInfo& vulkanImageInfo);
	void CreateImageView(const VulkanImageViewCreateInfo& vulkanImageViewInfo);

	uint32_t findMemoryTypeIndex(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties);
};
