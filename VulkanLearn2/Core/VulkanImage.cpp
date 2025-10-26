#include "pch.h"
#include "VulkanImage.h"
#include "VulkanBuffer.h"

// Create Vulkan Image
VulkanImage::VulkanImage(const VulkanHandles& vulkanHandles, const VulkanImageCreateInfo& imageCreateInfo, const VulkanImageViewCreateInfo& imageViewCreateInfo) :
	vk(vulkanHandles)
{
	CreateImage(imageCreateInfo);
	CreateImageView(imageViewCreateInfo);
}

// Create Vulkan Texture Image
VulkanImage::VulkanImage(const VulkanHandles& vulkanHandles, VulkanCommandManager* commandManager,const char* filePath, bool isCreateMipmap):
	vk(vulkanHandles)
{
	TextureImageInfo textureImageInfo{};
	textureImageInfo = loadImageFromFile(filePath);

	if (!isCreateMipmap)
	{
		textureImageInfo.textureImageMipLevels = 1;
	}

	CreateVulkanTextureImage(textureImageInfo);
	UploadDataToImage(commandManager, textureImageInfo);
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

	VK_CHECK(vkAllocateMemory(vk.device, &imageAllocInfo, nullptr, &handles.imageMemory), "FAILED TO ALLOCATE TEXTURE IMAGE MEMORY");

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

TextureImageInfo VulkanImage::loadImageFromFile(const char* filePath)
{
	int texWidth, texHeight, texChanels;
	//stbi_set_flip_vertically_on_load(true);
	stbi_uc* pixels = stbi_load(filePath, &texWidth, &texHeight, &texChanels, STBI_rgb_alpha);

	uint32_t textureImageMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels)
	{
		throw std::runtime_error(std::string("FAILED TO LOAD IMAGE FROM") + filePath);
	}
	else
	{
		std::cout << std::string("LOADED IMAGE FROM FILE: ") + filePath << std::endl;
	}

	TextureImageInfo textureImageInfo{texWidth, texHeight, texChanels, textureImageMipLevels, imageSize, pixels};
	return textureImageInfo;
}

void VulkanImage::CreateVulkanTextureImage(TextureImageInfo textureImageInfo)
{
	VulkanImageCreateInfo imageInfo{};
	imageInfo.width = textureImageInfo.texWidth;
	imageInfo.height = textureImageInfo.texHeight;
	imageInfo.mipLevels = textureImageInfo.textureImageMipLevels;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageInfo.imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; // cần DST để copy dữ liệu sang.
	imageInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VulkanImageViewCreateInfo imageViewInfo{};
	imageViewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageViewInfo.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewInfo.mipLevels = textureImageInfo.textureImageMipLevels;

	CreateImage(imageInfo);
	CreateImageView(imageViewInfo);
}

void VulkanImage::UploadDataToImage(VulkanCommandManager* cmd, TextureImageInfo& const textureImageInfo)
{

	// Create Staging buffer để làm trung gian upload ảnh.
	VulkanBuffer* stagingBuffer;

	VkBufferCreateInfo stagingInfo{};
	stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingInfo.queueFamilyIndexCount = 1;
	stagingInfo.pQueueFamilyIndices = &vk.queueFamilyIndices.GraphicQueueIndex;
	stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	stagingInfo.size = textureImageInfo.imageSize;
	stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	stagingBuffer = new VulkanBuffer(vk, cmd, stagingInfo);

	stagingBuffer->UploadData(textureImageInfo.pixels, textureImageInfo.imageSize, 0);
	// Giải phóng bộ nhớ của ảnh trên RAM vì đã upload.
	stbi_image_free(textureImageInfo.pixels);

	
	// Copy Data to Image
	// - Trans Image Layout 
	VkCommandBuffer cmdBuffer;
	cmdBuffer = cmd->BeginSingleTimeCmdBuffer();

	TransitionImageLayout(cmdBuffer, handles.image, textureImageInfo.textureImageMipLevels,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		-1, -1,
		0, VK_ACCESS_TRANSFER_WRITE_BIT
	);

	// - Copy
	VkBufferImageCopy region{};

	region.bufferImageHeight = 0;
	region.bufferRowLength = 0;
	region.bufferOffset = 0;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent.width = textureImageInfo.texWidth;
	region.imageExtent.height = textureImageInfo.texHeight;
	region.imageExtent.depth = 1;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.mipLevel = 0;
	
	vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer->getHandles().buffer, handles.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	cmd->EndSingleTimeCmdBuffer(cmdBuffer);

	// Create Mipmap
	if (textureImageInfo.textureImageMipLevels != 1)
	{
		GenerateMipMaps(cmd, handles.image, textureImageInfo.texWidth, textureImageInfo.texHeight, textureImageInfo.textureImageMipLevels);
	}
	else
	{
		VkCommandBuffer singleTimeCmd = cmd->BeginSingleTimeCmdBuffer();
		TransitionImageLayout(singleTimeCmd, handles.image, 1,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			-1, -1, 
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
		);
		cmd->EndSingleTimeCmdBuffer(singleTimeCmd);
	}
	// Cleanup
	delete(stagingBuffer);
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
	throw std::runtime_error("FAILED TO FIND SUITABLE MEMMORY TYPE");
}

void VulkanImage::TransitionImageLayout(VkCommandBuffer& cmdBuffer, VkImage& image, uint32_t mipLevels,
	VkImageLayout oldLayout, VkImageLayout newLayout,
	VkPipelineStageFlags srcStageFlag, VkPipelineStageFlags dstStageFlag,
	uint32_t baseMipLevel /*= -1*/, uint32_t levelCount /*= -1*/,
	VkAccessFlags srcAccessMask /*= 0*/, VkAccessFlags dstAccessMask /*= 0*/
)
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;

	barrier.newLayout = newLayout;
	barrier.oldLayout = oldLayout;

	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.srcAccessMask = srcAccessMask;
	barrier.dstAccessMask = dstAccessMask;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	barrier.subresourceRange.levelCount = levelCount == -1 ? mipLevels : levelCount;
	barrier.subresourceRange.baseMipLevel = baseMipLevel == -1 ? 0 : baseMipLevel;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;

	vkCmdPipelineBarrier(cmdBuffer,
		srcStageFlag,
		dstStageFlag,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}

void VulkanImage::GenerateMipMaps(VulkanCommandManager* cmd, VkImage image, uint32_t width, uint32_t height, uint32_t mipLevels)
{
	VkCommandBuffer cmdBuffer =cmd->BeginSingleTimeCmdBuffer();

	for (int i = 1; i < mipLevels; i++)
	{
		TransitionImageLayout(cmdBuffer, image, mipLevels,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			i - 1, 1,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);

		TransitionImageLayout(cmdBuffer, image, mipLevels,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			i, 1,
			0, VK_ACCESS_TRANSFER_WRITE_BIT);

		VkImageBlit blit{};
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.layerCount = 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.mipLevel = i - 1;

		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.layerCount = 1;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.mipLevel = i;

		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { static_cast<int32_t>(width / pow(2, i - 1)), static_cast<int32_t>(height / pow(2, i - 1)), 1 };
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { static_cast<int32_t>(width / pow(2, i)), static_cast<int32_t>(height / pow(2, i)), 1 };

		vkCmdBlitImage(cmdBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);


		TransitionImageLayout(cmdBuffer, image, mipLevels,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			i - 1, 1,
			VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT);
	}

	TransitionImageLayout(cmdBuffer, image, mipLevels,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		mipLevels - 1, 1,
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);


	cmd->EndSingleTimeCmdBuffer(cmdBuffer);
}

