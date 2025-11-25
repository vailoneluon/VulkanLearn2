#pragma once
#include "VulkanContext.h"

// Forward declarations
class VulkanBuffer;
class VulkanCommandManager;

// =================================================================================================
// Struct: TextureInfo
// Mô tả: Chứa thông tin chi tiết về một texture được tải từ file.
//        Lưu trữ các thuộc tính như kích thước, số kênh màu, số mipmap level,
//        kích thước dữ liệu và con trỏ tới dữ liệu pixel thô sau khi tải từ file.
// =================================================================================================
struct TextureInfo
{
	int width = 0;					// Chiều rộng của texture (pixel).
	int height = 0;					// Chiều cao của texture (pixel).
	int channels = 0;				// Số kênh màu của texture (ví dụ: 3 cho RGB, 4 cho RGBA).
	uint32_t mipLevels = 1;			// Số lượng mipmap level cho texture.
	VkDeviceSize size = 0;			// Kích thước tổng cộng của dữ liệu pixel (byte).
	stbi_uc* pixels = nullptr;		// Con trỏ tới dữ liệu pixel thô trong bộ nhớ RAM.
};

// =================================================================================================
// Struct: VulkanImageHandles
// Mô tả: Chứa các handle Vulkan cốt lõi liên quan đến một image và thông tin texture.
//        Đóng gói VkImage, VmaAllocation, VkImageView và TextureInfo (nếu là texture),
//        giúp quản lý tài nguyên image trong Vulkan một cách tập trung.
// =================================================================================================
struct VulkanImageHandles
{
	VkImage image = VK_NULL_HANDLE;				// Handle của VkImage.
	VmaAllocation allocation = VK_NULL_HANDLE;	// Handle cấp phát bộ nhớ từ VMA cho image.
	VkImageView imageView = VK_NULL_HANDLE;		// Handle của VkImageView.

	TextureInfo textureInfo; 					// Thông tin chi tiết về texture (chỉ dùng khi image là một texture).
};

// =================================================================================================
// Struct: VulkanImageCreateInfo
// Mô tả: Thông tin cần thiết để tạo một VkImage.
//        Định nghĩa các thuộc tính cơ bản của một image như kích thước, định dạng,
//        số mipmap, số sample và cờ sử dụng, giúp cấu hình image một cách linh hoạt.
// =================================================================================================
struct VulkanImageCreateInfo
{
	uint32_t width = 0;					// Chiều rộng của image.
	uint32_t height = 0;					// Chiều cao của image.
	uint32_t mipLevels = 1;				// Số lượng mipmap level.
	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;	// Số lượng sample cho multisampling.
	VkFormat format = VK_FORMAT_UNDEFINED;		// Định dạng pixel của image.
	VkImageUsageFlags imageUsageFlags = 0;		// Cờ sử dụng của image (ví dụ: VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT).
	VkMemoryPropertyFlags memoryFlags = 0;		// Cờ thuộc tính bộ nhớ (ví dụ: VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT).
};

// =================================================================================================
// Struct: VulkanImageViewCreateInfo
// Mô tả: Thông tin cần thiết để tạo một VkImageView.
//        Định nghĩa cách một VkImage sẽ được nhìn nhận và truy cập bởi pipeline,
//        bao gồm định dạng, các khía cạnh (aspect) và số mipmap level được hiển thị.
// =================================================================================================
struct VulkanImageViewCreateInfo
{
	VkFormat format = VK_FORMAT_UNDEFINED;		// Định dạng của image view.
	VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_NONE;	// Các khía cạnh của image view (ví dụ: VK_IMAGE_ASPECT_COLOR_BIT).
	uint32_t mipLevels = 1;				// Số lượng mipmap level mà view này có thể truy cập.
};

// =================================================================================================
// Class: VulkanImage
// Mô tả: 
//      Đóng gói và quản lý một VkImage cùng các tài nguyên liên quan (VkImageView, VmaAllocation).
//      Cung cấp các phương thức để tạo và quản lý các loại image khác nhau
//      như texture, depth buffer, color attachment, v.v.
//      Hỗ trợ tải texture từ file, tạo mipmap và chuyển đổi layout image.
// =================================================================================================
class VulkanImage
{
public:
	// Constructor: Cho các loại image chung (ví dụ: depth buffer, color attachment).
	// Tham số:
	//      vulkanHandles: Tham chiếu đến các handle Vulkan chung của ứng dụng.
	//      imageCI: Thông tin tạo VkImage.
	//      imageViewCI: Thông tin tạo VkImageView.
	VulkanImage(const VulkanHandles& vulkanHandles, const VulkanImageCreateInfo& imageCI, const VulkanImageViewCreateInfo& imageViewCI);

	// Constructor: Để tải texture từ file.
	// Tham số:
	//      vulkanHandles: Tham chiếu đến các handle Vulkan chung của ứng dụng.
	//      filePath: Đường dẫn đến file ảnh.
	//      createMipmaps: Có tạo mipmap cho texture hay không (mặc định là false).
	VulkanImage(const VulkanHandles& vulkanHandles, const char* filePath, VkFormat imageFormat, bool createMipmaps = false);

	// Destructor: Giải phóng VkImageView, VkImage và bộ nhớ đã cấp phát.
	~VulkanImage();

	// Cấm sao chép và gán để tránh quản lý tài nguyên sai lầm.
	VulkanImage(const VulkanImage&) = delete;
	VulkanImage& operator=(const VulkanImage&) = delete;

	// Phương thức: UploadTextureData
	// Mô tả: Tải dữ liệu pixel từ RAM lên VkImage thông qua một staging buffer.
	//        Thực hiện các bước:
	//        1. Tạo staging buffer và copy pixel từ RAM.
	//        2. Chuyển đổi layout image.
	//        3. Copy từ staging buffer sang image.
	//        4. Tạo mipmap (nếu cần).
	//        5. Chuyển đổi layout sang SHADER_READ_ONLY.
	// Tham số:
	//      cmdManager: Con trỏ tới CommandManager.
	//      cmdBuffer: Command buffer hiện tại.
	// Trả về: Con trỏ tới staging buffer đã tạo (Caller phải giải phóng).
	VulkanBuffer* UploadTextureData(VulkanCommandManager* cmdManager, VkCommandBuffer& cmdBuffer);

	// Getter: Lấy các handle và thông tin của image.
	const VulkanImageHandles& GetHandles() const { return m_Handles; }

	// Phương thức Static: TransitionLayout
	// Mô tả: Chuyển đổi layout của một image bằng pipeline barrier.
	// Tham số:
	//      cmdBuffer: Command buffer để ghi lệnh barrier.
	//      image: VkImage cần chuyển đổi layout.
	//      totalMipLevels: Tổng số mipmap level.
	//      oldLayout: Layout hiện tại.
	//      newLayout: Layout mong muốn.
	//      srcStage, dstStage: Các giai đoạn pipeline nguồn/đích.
	//      srcAccessMask, dstAccessMask: Các quyền truy cập nguồn/đích.
	static void TransitionLayout(
		const VkCommandBuffer& cmdBuffer,
		const VkImage& image,
		uint32_t totalMipLevels,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		VkPipelineStageFlags srcStage,
		VkPipelineStageFlags dstStage,
		VkAccessFlags srcAccessMask = 0,
		VkAccessFlags dstAccessMask = 0,
		uint32_t baseMipLevel = 0,
		uint32_t levelCount = 0
	);

private:
	// --- Dữ liệu nội bộ ---
	VulkanImageHandles m_Handles;			// Các handle và thông tin của image.
	const VulkanHandles& m_VulkanHandles;	// Tham chiếu đến các handle Vulkan chung.

	// --- Hàm khởi tạo và helper ---
	
	// Helper: Tạo một VkImage dựa trên thông tin cung cấp.
	void CreateImage(const VulkanImageCreateInfo& imageCI);

	// Helper: Tạo một VkImageView dựa trên thông tin cung cấp.
	void CreateImageView(const VulkanImageViewCreateInfo& imageViewCI);

	// Helper: Tải dữ liệu pixel từ file ảnh vào bộ nhớ RAM.
	void LoadImageDataFromFile(const char* filePath, bool createMipmaps);

	// Helper: Tạo mipmap cho VkImage đã cho.
	// Sử dụng lệnh vkCmdBlitImage để tạo các mipmap level.
	void GenerateMipmaps(VkCommandBuffer& cmdBuffer, VkImage image, uint32_t width, uint32_t height, uint32_t mipLevels);
};
