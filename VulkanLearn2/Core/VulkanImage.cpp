#include "pch.h"
#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "VulkanCommandManager.h"

// Constructor cho image chung (ví dụ: depth buffer, color attachment).
VulkanImage::VulkanImage(const VulkanHandles& vulkanHandles, const VulkanImageCreateInfo& imageCI, const VulkanImageViewCreateInfo& imageViewCI) :
	m_VulkanHandles(vulkanHandles)
{
	CreateImage(imageCI);
	CreateImageView(imageViewCI);
}

// Constructor để tải texture từ file.
VulkanImage::VulkanImage(const VulkanHandles& vulkanHandles, const char* filePath, bool createMipmaps) :
	m_VulkanHandles(vulkanHandles)
{
	// 1. Tải dữ liệu pixel từ file vào m_Handles.textureInfo
	LoadImageDataFromFile(filePath, createMipmaps);

	// 2. Tạo VkImage
	VulkanImageCreateInfo imageCI{};
	imageCI.width = m_Handles.textureInfo.width;
	imageCI.height = m_Handles.textureInfo.height;
	imageCI.mipLevels = m_Handles.textureInfo.mipLevels;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.format = VK_FORMAT_R8G8B8A8_SRGB;
	// Cần DST để copy dữ liệu từ staging buffer, và SRC để generate mipmaps.
	imageCI.imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageCI.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	CreateImage(imageCI);

	// 3. Tạo VkImageView
	VulkanImageViewCreateInfo imageViewCI{};
	imageViewCI.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageViewCI.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCI.mipLevels = m_Handles.textureInfo.mipLevels;
	CreateImageView(imageViewCI);
}

VulkanImage::~VulkanImage()
{
	vkDestroyImageView(m_VulkanHandles.device, m_Handles.imageView, nullptr);
	vkDestroyImage(m_VulkanHandles.device, m_Handles.image, nullptr);
	vkFreeMemory(m_VulkanHandles.device, m_Handles.imageMemory, nullptr);
}

void VulkanImage::CreateImage(const VulkanImageCreateInfo& imageCI)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.queueFamilyIndexCount = 1;
	imageInfo.pQueueFamilyIndices = &m_VulkanHandles.queueFamilyIndices.GraphicQueueIndex;
	imageInfo.extent.width = imageCI.width;
	imageInfo.extent.height = imageCI.height;
	imageInfo.extent.depth = 1;
	imageInfo.format = imageCI.format;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.mipLevels = imageCI.mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = imageCI.samples;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = imageCI.imageUsageFlags;

	VK_CHECK(vkCreateImage(m_VulkanHandles.device, &imageInfo, nullptr, &m_Handles.image), "LỖI: Tạo VkImage thất bại!");

	// Cấp phát bộ nhớ cho image
	VkMemoryRequirements memRequirements{};
	vkGetImageMemoryRequirements(m_VulkanHandles.device, m_Handles.image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryTypeIndex(memRequirements.memoryTypeBits, imageCI.memoryFlags);

	VK_CHECK(vkAllocateMemory(m_VulkanHandles.device, &allocInfo, nullptr, &m_Handles.imageMemory), "LỖI: Cấp phát bộ nhớ cho VkImage thất bại!");

	// Bind bộ nhớ vào image
	vkBindImageMemory(m_VulkanHandles.device, m_Handles.image, m_Handles.imageMemory, 0);
}

void VulkanImage::CreateImageView(const VulkanImageViewCreateInfo& imageViewCI)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_Handles.image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = imageViewCI.format;
	viewInfo.subresourceRange.aspectMask = imageViewCI.aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = imageViewCI.mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VK_CHECK(vkCreateImageView(m_VulkanHandles.device, &viewInfo, nullptr, &m_Handles.imageView), "LỖI: Tạo VkImageView thất bại!");
}

void VulkanImage::LoadImageDataFromFile(const char* filePath, bool createMipmaps)
{
	stbi_set_flip_vertically_on_load(true); // Vulkan có hệ tọa độ Y ngược với nhiều API khác
	stbi_uc* pixels = stbi_load(filePath, &m_Handles.textureInfo.width, &m_Handles.textureInfo.height, &m_Handles.textureInfo.channels, STBI_rgb_alpha);

	if (!pixels)
	{
		throw std::runtime_error(std::string("LỖI: Tải ảnh thất bại từ: ") + filePath);
	}
	
	std::cout << "Tải ảnh thành công từ: " << filePath << std::endl;

	m_Handles.textureInfo.pixels = pixels;
	m_Handles.textureInfo.size = m_Handles.textureInfo.width * m_Handles.textureInfo.height * 4;
	
	if (createMipmaps)
	{
		m_Handles.textureInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_Handles.textureInfo.width, m_Handles.textureInfo.height)))) + 1;
	}
	else
	{
		m_Handles.textureInfo.mipLevels = 1;
	}
}

VulkanBuffer* VulkanImage::UploadTextureData(VulkanCommandManager* cmdManager, VkCommandBuffer& cmdBuffer)
{
	// --- 1. Tạo Staging Buffer ---
	// Staging buffer là một buffer trung gian trên CPU-visible memory để tải dữ liệu lên.
	// Sau đó dữ liệu sẽ được copy từ staging buffer sang device-local memory (nhanh hơn) của image.
	VulkanBuffer* stagingBuffer;

	VkBufferCreateInfo stagingInfo{};
	stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingInfo.queueFamilyIndexCount = 1;
	stagingInfo.pQueueFamilyIndices = &m_VulkanHandles.queueFamilyIndices.GraphicQueueIndex;
	stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	stagingInfo.size = m_Handles.textureInfo.size;
	stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	// Tải texture lên staging Buffer.
	stagingBuffer = new VulkanBuffer(m_VulkanHandles, cmdManager, stagingInfo, false);
	stagingBuffer->UploadData(m_Handles.textureInfo.pixels, m_Handles.textureInfo.size, 0);
	
	// Dữ liệu pixel đã được copy vào staging buffer, có thể giải phóng bộ nhớ trên RAM.
	stbi_image_free(m_Handles.textureInfo.pixels);
	m_Handles.textureInfo.pixels = nullptr;

	// --- 2. Copy dữ liệu từ Staging Buffer sang Image ---
	
	// Chuyển layout của image từ UNDEFINED -> TRANSFER_DST_OPTIMAL để nhận dữ liệu.
	TransitionLayout(cmdBuffer, m_Handles.image, m_Handles.textureInfo.mipLevels,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, VK_ACCESS_TRANSFER_WRITE_BIT, 0, m_Handles.textureInfo.mipLevels
	);

	// Thực hiện copy
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { (uint32_t)m_Handles.textureInfo.width, (uint32_t)m_Handles.textureInfo.height, 1 };
	
	vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer->getHandles().buffer, m_Handles.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	// --- 3. Generate Mipmaps ---
	// Mipmap được tạo từ mip level 0. Sau khi copy, layout của mip 0 là TRANSFER_DST.
	// Các mip level khác vẫn là UNDEFINED.
	if (m_Handles.textureInfo.mipLevels > 1)
	{
		GenerateMipmaps(cmdBuffer, m_Handles.image, m_Handles.textureInfo.width, m_Handles.textureInfo.height, m_Handles.textureInfo.mipLevels);
	}
	else
	{
		// Nếu không có mipmap, chỉ cần chuyển layout sang SHADER_READ_ONLY để shader có thể đọc.
		TransitionLayout(cmdBuffer, m_Handles.image, m_Handles.textureInfo.mipLevels,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
		);
	}
	
	// QUAN TRỌNG: Staging buffer được trả về và phải được giải phóng bởi caller
	// sau khi command buffer đã thực thi xong.
	return stagingBuffer;
}

uint32_t VulkanImage::FindMemoryTypeIndex(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_VulkanHandles.physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	throw std::runtime_error("LỖI: Không tìm thấy memory type phù hợp!");
}

void VulkanImage::TransitionLayout(
	VkCommandBuffer& cmdBuffer, VkImage& image, uint32_t totalMipLevels,
	VkImageLayout oldLayout, VkImageLayout newLayout,
	VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
	VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
	uint32_t baseMipLevel, uint32_t levelCount)
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.srcAccessMask = srcAccessMask;
	barrier.dstAccessMask = dstAccessMask;

	// Subresource range
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		// TODO: check if format has stencil component
		// if (true) 
		// {
		// 	barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		// }
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	
	barrier.subresourceRange.baseMipLevel = baseMipLevel;
	// Nếu levelCount là 0, nó sẽ áp dụng cho tất cả các mip level còn lại.
	barrier.subresourceRange.levelCount = (levelCount == 0) ? (totalMipLevels - baseMipLevel) : levelCount;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier(cmdBuffer,
		srcStage, dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}

void VulkanImage::GenerateMipmaps(VkCommandBuffer& cmdBuffer, VkImage image, uint32_t width, uint32_t height, uint32_t mipLevels)
{
	// TODO: Đảm bảo physical device hỗ trợ blitting với linear filter
	// (Cần check trong lúc khởi tạo VulkanContext)

	int32_t mipWidth = width;
	int32_t mipHeight = height;

	// Vòng lặp bắt đầu từ mip level 1 (vì level 0 là ảnh gốc)
	for (uint32_t i = 1; i < mipLevels; i++)
	{
		// Chuyển layout của mip level (i-1) sang SRC_OPTIMAL để làm nguồn cho blit
		TransitionLayout(cmdBuffer, image, mipLevels,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			i - 1, 1);

		// Thực hiện blit từ mip (i-1) sang mip (i)
		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;

		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(cmdBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		// Chuyển layout của mip (i-1) sang SHADER_READ_ONLY vì nó đã xong việc
		TransitionLayout(cmdBuffer, image, mipLevels,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
			i - 1, 1);

		// Cập nhật kích thước cho mip level tiếp theo
		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	// Chuyển layout của mip level cuối cùng sang SHADER_READ_ONLY
	TransitionLayout(cmdBuffer, image, mipLevels,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		mipLevels - 1, 1);
}