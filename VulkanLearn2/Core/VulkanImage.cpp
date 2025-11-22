#include "pch.h"
#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "VulkanCommandManager.h"

/**
 * @brief Constructor cho image chung (ví dụ: depth buffer, color attachment).
 * @param vulkanHandles Tham chiếu đến các handle Vulkan chung của ứng dụng.
 * @param imageCI Thông tin tạo VkImage.
 * @param imageViewCI Thông tin tạo VkImageView.
 */
VulkanImage::VulkanImage(const VulkanHandles& vulkanHandles, const VulkanImageCreateInfo& imageCI, const VulkanImageViewCreateInfo& imageViewCI)
	: m_VulkanHandles(vulkanHandles)
{
	CreateImage(imageCI);
	CreateImageView(imageViewCI);
}

/**
 * @brief Constructor để tải texture từ file.
 * @param vulkanHandles Tham chiếu đến các handle Vulkan chung của ứng dụng.
 * @param filePath Đường dẫn đến file ảnh.
 * @param createMipmaps Có tạo mipmap cho texture hay không.
 */
VulkanImage::VulkanImage(const VulkanHandles& vulkanHandles, const char* filePath, VkFormat imageFormat, bool createMipmaps)
	: m_VulkanHandles(vulkanHandles)
{
	// 1. Tải dữ liệu pixel từ file vào RAM và lưu thông tin vào m_Handles.textureInfo.
	LoadImageDataFromFile(filePath, createMipmaps);

	// 2. Tạo VkImage dựa trên thông tin texture đã tải.
	VulkanImageCreateInfo imageCI{};
	imageCI.width = m_Handles.textureInfo.width;
	imageCI.height = m_Handles.textureInfo.height;
	imageCI.mipLevels = m_Handles.textureInfo.mipLevels;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.format = imageFormat;
	// Image cần cờ VK_IMAGE_USAGE_TRANSFER_DST_BIT để nhận dữ liệu từ staging buffer
	// và VK_IMAGE_USAGE_TRANSFER_SRC_BIT để làm nguồn cho việc tạo mipmap (blit).
	imageCI.imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageCI.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	CreateImage(imageCI);

	// 3. Tạo VkImageView để truy cập image.
	VulkanImageViewCreateInfo imageViewCI{};
	imageViewCI.format = imageFormat;
	imageViewCI.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCI.mipLevels = m_Handles.textureInfo.mipLevels;
	CreateImageView(imageViewCI);
}

/**
 * @brief Destructor, giải phóng VkImageView, VkImage và bộ nhớ đã cấp phát.
 */
VulkanImage::~VulkanImage()
{
	// Hủy VkImageView.
	vkDestroyImageView(m_VulkanHandles.device, m_Handles.imageView, nullptr);
	// Hủy VkImage và giải phóng bộ nhớ đã cấp phát bởi VMA.
	vmaDestroyImage(m_VulkanHandles.allocator, m_Handles.image, m_Handles.allocation);
}

/**
 * @brief Tạo một VkImage dựa trên thông tin cung cấp.
 * @param imageCI Thông tin tạo VkImage.
 */
void VulkanImage::CreateImage(const VulkanImageCreateInfo& imageCI)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = imageCI.width;
	imageInfo.extent.height = imageCI.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = imageCI.mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = imageCI.format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = imageCI.imageUsageFlags;
	imageInfo.samples = imageCI.samples;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.queueFamilyIndexCount = 1;
	imageInfo.pQueueFamilyIndices = &m_VulkanHandles.queueFamilyIndices.GraphicQueueIndex;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY; // Image thường nằm trên bộ nhớ GPU để tối ưu hiệu suất.

	VmaAllocationInfo allocResultInfo;
	VK_CHECK(vmaCreateImage(m_VulkanHandles.allocator, &imageInfo, &allocInfo, &m_Handles.image, &m_Handles.allocation, &allocResultInfo),
		"Lỗi: Không thể tạo Vulkan Image!");
}

/**
 * @brief Tạo một VkImageView dựa trên thông tin cung cấp.
 * @param imageViewCI Thông tin tạo VkImageView.
 */
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

	VK_CHECK(vkCreateImageView(m_VulkanHandles.device, &viewInfo, nullptr, &m_Handles.imageView), "Lỗi: Tạo VkImageView thất bại!");
}

/**
 * @brief Tải dữ liệu pixel từ file ảnh vào bộ nhớ RAM.
 * @param filePath Đường dẫn đến file ảnh.
 * @param createMipmaps Có tạo mipmap cho texture hay không.
 */
void VulkanImage::LoadImageDataFromFile(const char* filePath, bool createMipmaps)
{
	// Kiểm tra file có tồn tại không.
	if (!std::filesystem::exists(filePath))
	{
		throw std::runtime_error("Error: Image file not found: " + std::string(filePath));
	}

	// Đặt cờ để lật ảnh theo chiều dọc vì Vulkan có hệ tọa độ Y ngược với nhiều API khác (ví dụ: OpenGL).
	stbi_set_flip_vertically_on_load(true);
	// Tải dữ liệu pixel từ file sử dụng thư viện stb_image.
	stbi_uc* pixels = stbi_load(filePath, &m_Handles.textureInfo.width, &m_Handles.textureInfo.height, &m_Handles.textureInfo.channels, STBI_rgb_alpha);

	// Kiểm tra việc đọc file có thành công không.
	if (!pixels)
	{
		throw std::runtime_error("Error: Failed to load image data from file: " + std::string(filePath) + ". File may be corrupt or in an unsupported format.");
	}

	//std::cout << "Loaded Image: " << filePath << std::endl;

	m_Handles.textureInfo.pixels = pixels;
	// Kích thước dữ liệu pixel (width * height * 4 bytes/pixel cho định dạng RGBA).
	m_Handles.textureInfo.size = static_cast<VkDeviceSize>(m_Handles.textureInfo.width) * m_Handles.textureInfo.height * 4;

	// Tính toán số lượng mipmap level nếu được yêu cầu.
	if (createMipmaps)
	{
		m_Handles.textureInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_Handles.textureInfo.width, m_Handles.textureInfo.height)))) + 1;
	}
	else
	{
		m_Handles.textureInfo.mipLevels = 1;
	}
}

/**
 * @brief Tải dữ liệu pixel từ RAM lên VkImage thông qua một staging buffer.
 * @param cmdManager Con trỏ tới CommandManager để thực hiện các lệnh sao chép và chuyển đổi layout.
 * @param cmdBuffer Command buffer hiện tại để ghi các lệnh Vulkan.
 * @return Con trỏ tới staging buffer đã tạo. Caller có trách nhiệm giải phóng staging buffer này
 *         sau khi command buffer đã thực thi xong trên GPU.
 */
VulkanBuffer* VulkanImage::UploadTextureData(VulkanCommandManager* cmdManager, VkCommandBuffer& cmdBuffer)
{
	// --- 1. Tạo Staging Buffer ---
	// Staging buffer là một buffer trung gian trên bộ nhớ CPU-visible để tải dữ liệu lên GPU.
	// Dữ liệu sẽ được copy từ staging buffer sang bộ nhớ device-local (GPU) của image để tối ưu hiệu suất.
	VulkanBuffer* stagingBuffer;

	VkBufferCreateInfo stagingInfo{};
	stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingInfo.size = m_Handles.textureInfo.size;
	stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; // Staging buffer sẽ là nguồn cho lệnh sao chép.
	stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	stagingInfo.queueFamilyIndexCount = 1;
	stagingInfo.pQueueFamilyIndices = &m_VulkanHandles.queueFamilyIndices.GraphicQueueIndex;

	// Tạo staging buffer và tải dữ liệu pixel từ RAM vào đó.
	stagingBuffer = new VulkanBuffer(m_VulkanHandles, cmdManager, stagingInfo, VMA_MEMORY_USAGE_CPU_TO_GPU);
	stagingBuffer->UploadData(m_Handles.textureInfo.pixels, m_Handles.textureInfo.size, 0);

	// Dữ liệu pixel đã được sao chép vào staging buffer, có thể giải phóng bộ nhớ trên RAM.
	stbi_image_free(m_Handles.textureInfo.pixels);
	m_Handles.textureInfo.pixels = nullptr;

	// --- 2. Copy dữ liệu từ Staging Buffer sang Image ---

	// Chuyển đổi layout của image từ VK_IMAGE_LAYOUT_UNDEFINED sang VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	// để image sẵn sàng nhận dữ liệu từ staging buffer.
	TransitionLayout(cmdBuffer, m_Handles.image, m_Handles.textureInfo.mipLevels,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, VK_ACCESS_TRANSFER_WRITE_BIT, 0, m_Handles.textureInfo.mipLevels
	);

	// Định nghĩa vùng dữ liệu cần sao chép.
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

	// Thực hiện lệnh sao chép từ buffer sang image.
	vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer->GetHandles().buffer, m_Handles.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	// --- 3. Tạo Mipmaps (nếu có) ---
	// Mipmap được tạo từ mip level 0. Sau khi copy, layout của mip 0 là TRANSFER_DST.
	// Các mip level khác vẫn là UNDEFINED hoặc chưa được sử dụng.
	if (m_Handles.textureInfo.mipLevels > 1)
	{
		GenerateMipmaps(cmdBuffer, m_Handles.image, m_Handles.textureInfo.width, m_Handles.textureInfo.height, m_Handles.textureInfo.mipLevels);
	}
	else
	{
		// Nếu không có mipmap, chỉ cần chuyển layout sang SHADER_READ_ONLY_OPTIMAL để shader có thể đọc.
		TransitionLayout(cmdBuffer, m_Handles.image, m_Handles.textureInfo.mipLevels,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
		);
	}

	// QUAN TRỌNG: Staging buffer được trả về và phải được giải phóng bởi caller
	// sau khi command buffer đã thực thi xong trên GPU.
	return stagingBuffer;
}

/**
 * @brief Chuyển đổi layout của một image bằng pipeline barrier.
 * @param cmdBuffer Command buffer để ghi lệnh barrier.
 * @param image VkImage cần chuyển đổi layout.
 * @param totalMipLevels Tổng số mipmap level của image.
 * @param oldLayout Layout hiện tại của image.
 * @param newLayout Layout mong muốn sau khi chuyển đổi.
 * @param srcStage Các giai đoạn pipeline nguồn mà barrier sẽ chờ.
 * @param dstStage Các giai đoạn pipeline đích mà barrier sẽ mở khóa.
 * @param srcAccessMask Các quyền truy cập nguồn mà barrier sẽ chờ.
 * @param dstAccessMask Các quyền truy cập đích mà barrier sẽ mở khóa.
 * @param baseMipLevel Mipmap level bắt đầu áp dụng barrier.
 * @param levelCount Số lượng mipmap level áp dụng barrier (0 có nghĩa là tất cả các level từ baseMipLevel).
 */
void VulkanImage::TransitionLayout(
	const VkCommandBuffer& cmdBuffer,const VkImage& image, uint32_t totalMipLevels,
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

	// Cấu hình subresourceRange dựa trên layout mới.
	// Nếu là depth/stencil attachment, sử dụng VK_IMAGE_ASPECT_DEPTH_BIT.
	// TODO: Cần kiểm tra định dạng có stencil component hay không để thêm VK_IMAGE_ASPECT_STENCIL_BIT.
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	barrier.subresourceRange.baseMipLevel = baseMipLevel;
	// Nếu levelCount là 0, áp dụng cho tất cả các mip level còn lại từ baseMipLevel.
	barrier.subresourceRange.levelCount = (levelCount == 0) ? (totalMipLevels - baseMipLevel) : levelCount;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	// Ghi lệnh pipeline barrier vào command buffer.
	vkCmdPipelineBarrier(cmdBuffer,
		srcStage, dstStage,
		0, // dependencyFlags
		0, nullptr, // memoryBarriers
		0, nullptr, // bufferMemoryBarriers
		1, &barrier // imageMemoryBarriers
	);
}

/**
 * @brief Tạo mipmap cho VkImage đã cho.
 * @param cmdBuffer Command buffer để ghi các lệnh tạo mipmap.
 * @param image VkImage cần tạo mipmap.
 * @param width Chiều rộng của mip level gốc (level 0).
 * @param height Chiều cao của mip level gốc (level 0).
 * @param mipLevels Tổng số mipmap level cần tạo.
 */
void VulkanImage::GenerateMipmaps(VkCommandBuffer& cmdBuffer, VkImage image, uint32_t width, uint32_t height, uint32_t mipLevels)
{
	// TODO: Đảm bảo physical device hỗ trợ blitting với linear filter.
	// (Cần kiểm tra trong quá trình khởi tạo VulkanContext để đảm bảo tính tương thích).

	int32_t mipWidth = static_cast<int32_t>(width);
	int32_t mipHeight = static_cast<int32_t>(height);

	// Vòng lặp bắt đầu từ mip level 1 (vì level 0 là ảnh gốc đã có dữ liệu).
	for (uint32_t i = 1; i < mipLevels; i++)
	{
		// Chuyển đổi layout của mip level (i-1) sang VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
		// để nó có thể làm nguồn cho lệnh blit.
		TransitionLayout(cmdBuffer, image, mipLevels,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			i - 1, 1);

		// Thực hiện lệnh blit (sao chép và thay đổi kích thước) từ mip (i-1) sang mip (i).
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
			VK_FILTER_LINEAR); // Sử dụng bộ lọc tuyến tính để làm mịn mipmap.

		// Chuyển đổi layout của mip (i-1) sang VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		// vì nó đã hoàn thành vai trò là nguồn và sẵn sàng cho shader đọc.
		TransitionLayout(cmdBuffer, image, mipLevels,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
			i - 1, 1);

		// Cập nhật kích thước cho mip level tiếp theo.
		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	// Sau khi vòng lặp kết thúc, mip level cuối cùng (mipLevels - 1) vẫn đang ở
	// VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL. Cần chuyển nó sang SHADER_READ_ONLY_OPTIMAL.
	TransitionLayout(cmdBuffer, image, mipLevels,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		mipLevels - 1, 1);
}
