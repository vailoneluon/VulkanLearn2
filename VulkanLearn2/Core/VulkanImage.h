#pragma once
#include "VulkanContext.h"

class VulkanBuffer;
class VulkanCommandManager;

// Thông tin về một texture được tải từ file.
struct TextureInfo
{
	int width = 0;
	int height = 0;
	int channels = 0;
	uint32_t mipLevels = 1;
	VkDeviceSize size = 0;
	stbi_uc* pixels = nullptr;
};

// Chứa các handle của Vulkan cho một image.
struct VulkanImageHandles
{
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory imageMemory = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;

	TextureInfo textureInfo; // Chỉ dùng khi image là một texture
};

// Thông tin để tạo VkImage.
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

// Thông tin để tạo VkImageView.
struct VulkanImageViewCreateInfo
{
	VkFormat format = VK_FORMAT_UNDEFINED;
	VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_NONE;
	uint32_t mipLevels = 1;
};

// Class VulkanImage đóng gói một VkImage và các tài nguyên liên quan (VkImageView, VkDeviceMemory).
// Có thể được sử dụng cho texture, depth buffer, color attachment, v.v.
class VulkanImage
{
public:
	// Constructor cho image chung (ví dụ: depth buffer, color attachment).
	VulkanImage(const VulkanHandles& vulkanHandles, const VulkanImageCreateInfo& imageCI, const VulkanImageViewCreateInfo& imageViewCI);
	// Constructor để tải texture từ file.
	VulkanImage(const VulkanHandles& vulkanHandles, const char* filePath, bool createMipmaps = false);
	~VulkanImage();

	// Tải dữ liệu pixel vào image thông qua một staging buffer.
	// Trả về staging buffer, caller có trách nhiệm giải phóng nó sau khi command buffer thực thi xong.
	VulkanBuffer* UploadTextureData(VulkanCommandManager* cmdManager, VkCommandBuffer& cmdBuffer);

	// Getter
	const VulkanImageHandles& GetHandles() const { return m_Handles; }

	// --- Static Helpers ---
	
	// Chuyển đổi layout của một image bằng pipeline barrier.
	static void TransitionLayout(
		VkCommandBuffer& cmdBuffer, 
		VkImage& image, 
		uint32_t totalMipLevels,
		VkImageLayout oldLayout, 
		VkImageLayout newLayout,
		VkPipelineStageFlags srcStage, 
		VkPipelineStageFlags dstStage,
		VkAccessFlags srcAccessMask = 0, 
		VkAccessFlags dstAccessMask = 0,
		uint32_t baseMipLevel = 0,
		uint32_t levelCount = 0 // 0 có nghĩa là tất cả các level từ baseMipLevel
	);

private:
	VulkanImageHandles m_Handles;
	const VulkanHandles& m_VulkanHandles;

	// --- Hàm khởi tạo ---
	void CreateImage(const VulkanImageCreateInfo& imageCI);
	void CreateImageView(const VulkanImageViewCreateInfo& imageViewCI);
	
	// --- Các hàm helper cho Texture ---
	void LoadImageDataFromFile(const char* filePath, bool createMipmaps);

	// --- Các hàm helper chung ---
	uint32_t FindMemoryTypeIndex(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties);
	void GenerateMipmaps(VkCommandBuffer& cmdBuffer, VkImage image, uint32_t width, uint32_t height, uint32_t mipLevels);
};