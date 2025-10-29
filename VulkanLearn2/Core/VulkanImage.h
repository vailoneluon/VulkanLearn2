#pragma once
#include "VulkanContext.h"
#include "VulkanCommandManager.h"

class VulkanBuffer;

struct TextureImageInfo
{
	int texWidth, texHeight, texChanels;
	uint32_t textureImageMipLevels;
	VkDeviceSize imageSize;
	stbi_uc* pixels;
};

struct ImageHandles
{
	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;

	TextureImageInfo imageInfo;
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
	VulkanImage(const VulkanHandles& vulkanHandles, VulkanCommandManager* commandManager,const char* filePath, bool isCreateMipmap = false);
	~VulkanImage();

	VulkanBuffer* UploadDataToImage(VulkanCommandManager* cmd, VkCommandBuffer& cmdBuffer, const TextureImageInfo& textureImageInfo);

	const ImageHandles& getHandles() const{ return handles; }
private:
	ImageHandles handles;
	const VulkanHandles& vk;

	void CreateImage(const VulkanImageCreateInfo& vulkanImageInfo);
	void CreateImageView(const VulkanImageViewCreateInfo& vulkanImageViewInfo);


	// Create Texture Image
	TextureImageInfo loadImageFromFile(const char* filePath);
	void CreateVulkanTextureImage(TextureImageInfo textureImageInfo);

	// Helper Function
	uint32_t findMemoryTypeIndex(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties);
	
	void TransitionImageLayout(VkCommandBuffer& cmdBuffer, VkImage& image, uint32_t mipLevels,
		VkImageLayout oldLayout, VkImageLayout newLayout,
		VkPipelineStageFlags srcStageFlag, VkPipelineStageFlags dstStageFlag,
		uint32_t baseMipLevel = -1, uint32_t levelCount = -1,
		VkAccessFlags srcAccessMask = 0, VkAccessFlags dstAccessMask = 0);

	void GenerateMipMaps(VulkanCommandManager* cmd, VkCommandBuffer& cmdBuffer, VkImage image, uint32_t width, uint32_t height, uint32_t mipLevels);
};
