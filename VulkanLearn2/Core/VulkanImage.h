#pragma once
#include "VulkanContext.h"

// Forward declarations
class VulkanBuffer;
class VulkanCommandManager;

/**
 * @struct TextureInfo
 * @brief Chứa thông tin chi tiết về một texture được tải từ file.
 *
 * Struct này lưu trữ các thuộc tính như kích thước, số kênh màu, số mipmap level,
 * kích thước dữ liệu và con trỏ tới dữ liệu pixel thô sau khi tải từ file.
 */
struct TextureInfo
{
	int width = 0;					///< Chiều rộng của texture (pixel).
	int height = 0;					///< Chiều cao của texture (pixel).
	int channels = 0;				///< Số kênh màu của texture (ví dụ: 3 cho RGB, 4 cho RGBA).
	uint32_t mipLevels = 1;			///< Số lượng mipmap level cho texture.
	VkDeviceSize size = 0;			///< Kích thước tổng cộng của dữ liệu pixel (byte).
	stbi_uc* pixels = nullptr;		///< Con trỏ tới dữ liệu pixel thô trong bộ nhớ RAM.
};

/**
 * @struct VulkanImageHandles
 * @brief Chứa các handle Vulkan cốt lõi liên quan đến một image và thông tin texture.
 *
 * Struct này đóng gói VkImage, VmaAllocation, VkImageView và TextureInfo (nếu là texture),
 * giúp quản lý tài nguyên image trong Vulkan một cách tập trung.
 */
struct VulkanImageHandles
{
	VkImage image = VK_NULL_HANDLE;			///< Handle của VkImage.
	VmaAllocation allocation = VK_NULL_HANDLE;	///< Handle cấp phát bộ nhớ từ VMA cho image.
	VkImageView imageView = VK_NULL_HANDLE;	///< Handle của VkImageView.

	TextureInfo textureInfo; 				///< Thông tin chi tiết về texture (chỉ dùng khi image là một texture).
};

/**
 * @struct VulkanImageCreateInfo
 * @brief Thông tin cần thiết để tạo một VkImage.
 *
 * Struct này định nghĩa các thuộc tính cơ bản của một image như kích thước, định dạng,
 * số mipmap, số sample và cờ sử dụng, giúp cấu hình image một cách linh hoạt.
 */
struct VulkanImageCreateInfo
{
	uint32_t width = 0;					///< Chiều rộng của image.
	uint32_t height = 0;					///< Chiều cao của image.
	uint32_t mipLevels = 1;				///< Số lượng mipmap level.
	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;	///< Số lượng sample cho multisampling.
	VkFormat format = VK_FORMAT_UNDEFINED;		///< Định dạng pixel của image.
	VkImageUsageFlags imageUsageFlags = 0;		///< Cờ sử dụng của image (ví dụ: VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT).
	VkMemoryPropertyFlags memoryFlags = 0;		///< Cờ thuộc tính bộ nhớ (ví dụ: VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT).
};

/**
 * @struct VulkanImageViewCreateInfo
 * @brief Thông tin cần thiết để tạo một VkImageView.
 *
 * Struct này định nghĩa cách một VkImage sẽ được nhìn nhận và truy cập bởi pipeline,
 * bao gồm định dạng, các khía cạnh (aspect) và số mipmap level được hiển thị.
 */
struct VulkanImageViewCreateInfo
{
	VkFormat format = VK_FORMAT_UNDEFINED;		///< Định dạng của image view.
	VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_NONE;	///< Các khía cạnh của image view (ví dụ: VK_IMAGE_ASPECT_COLOR_BIT).
	uint32_t mipLevels = 1;				///< Số lượng mipmap level mà view này có thể truy cập.
};

/**
 * @class VulkanImage
 * @brief Đóng gói và quản lý một VkImage cùng các tài nguyên liên quan (VkImageView, VmaAllocation).
 *
 * Class này cung cấp các phương thức để tạo và quản lý các loại image khác nhau
 * như texture, depth buffer, color attachment, v.v. Nó hỗ trợ tải texture từ file,
 * tạo mipmap và chuyển đổi layout image.
 */
class VulkanImage
{
public:
	/**
	 * @brief Constructor cho các loại image chung (ví dụ: depth buffer, color attachment).
	 * @param vulkanHandles Tham chiếu đến các handle Vulkan chung của ứng dụng.
	 * @param imageCI Thông tin tạo VkImage.
	 * @param imageViewCI Thông tin tạo VkImageView.
	 */
	VulkanImage(const VulkanHandles& vulkanHandles, const VulkanImageCreateInfo& imageCI, const VulkanImageViewCreateInfo& imageViewCI);

	/**
	 * @brief Constructor để tải texture từ file.
	 * @param vulkanHandles Tham chiếu đến các handle Vulkan chung của ứng dụng.
	 * @param filePath Đường dẫn đến file ảnh.
	 * @param createMipmaps Có tạo mipmap cho texture hay không (mặc định là false).
	 */
	VulkanImage(const VulkanHandles& vulkanHandles, const char* filePath, VkFormat imageFormat, bool createMipmaps = false);

	/**
	 * @brief Destructor, giải phóng VkImageView, VkImage và bộ nhớ đã cấp phát.
	 */
	~VulkanImage();

	// Cấm sao chép và gán để tránh quản lý tài nguyên sai lầm.
	VulkanImage(const VulkanImage&) = delete;
	VulkanImage& operator=(const VulkanImage&) = delete;

	/**
	 * @brief Tải dữ liệu pixel từ RAM lên VkImage thông qua một staging buffer.
	 *
	 * Phương thức này thực hiện các bước sau:
	 * 1. Tạo một staging buffer và sao chép dữ liệu pixel từ RAM vào đó.
	 * 2. Chuyển đổi layout của VkImage để sẵn sàng nhận dữ liệu.
	 * 3. Sao chép dữ liệu từ staging buffer vào VkImage.
	 * 4. Tạo mipmap (nếu được yêu cầu).
	 * 5. Chuyển đổi layout của VkImage sang trạng thái sẵn sàng cho shader đọc.
	 *
	 * @param cmdManager Con trỏ tới CommandManager để thực hiện các lệnh sao chép và chuyển đổi layout.
	 * @param cmdBuffer Command buffer hiện tại để ghi các lệnh Vulkan.
	 * @return Con trỏ tới staging buffer đã tạo. Caller có trách nhiệm giải phóng staging buffer này
	 *         sau khi command buffer đã thực thi xong trên GPU.
	 */
	VulkanBuffer* UploadTextureData(VulkanCommandManager* cmdManager, VkCommandBuffer& cmdBuffer);

	/**
	 * @brief Lấy các handle và thông tin của image.
	 * @return Tham chiếu hằng tới struct VulkanImageHandles.
	 */
	const VulkanImageHandles& GetHandles() const { return m_Handles; }

	/**
	 * @brief Chuyển đổi layout của một image bằng pipeline barrier.
	 *
	 * Phương thức static này tạo một VkImageMemoryBarrier để đồng bộ hóa và chuyển đổi
	 * trạng thái layout của image giữa các hoạt động Vulkan khác nhau.
	 *
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
	VulkanImageHandles m_Handles;			///< Các handle và thông tin của image.
	const VulkanHandles& m_VulkanHandles;	///< Tham chiếu đến các handle Vulkan chung.

	// --- Hàm khởi tạo và helper ---
	/**
	 * @brief Tạo một VkImage dựa trên thông tin cung cấp.
	 * @param imageCI Thông tin tạo VkImage.
	 */
	void CreateImage(const VulkanImageCreateInfo& imageCI);

	/**
	 * @brief Tạo một VkImageView dựa trên thông tin cung cấp.
	 * @param imageViewCI Thông tin tạo VkImageView.
	 */
	void CreateImageView(const VulkanImageViewCreateInfo& imageViewCI);

	/**
	 * @brief Tải dữ liệu pixel từ file ảnh vào bộ nhớ RAM.
	 * @param filePath Đường dẫn đến file ảnh.
	 * @param createMipmaps Có tạo mipmap cho texture hay không.
	 */
	void LoadImageDataFromFile(const char* filePath, bool createMipmaps);

	/**
	 * @brief Tạo mipmap cho VkImage đã cho.
	 *
	 * Phương thức này sử dụng lệnh `vkCmdBlitImage` để tạo các mipmap level
	 * từ mip level gốc, chuyển đổi layout image khi cần thiết.
	 *
	 * @param cmdBuffer Command buffer để ghi các lệnh tạo mipmap.
	 * @param image VkImage cần tạo mipmap.
	 * @param width Chiều rộng của mip level gốc (level 0).
	 * @param height Chiều cao của mip level gốc (level 0).
	 * @param mipLevels Tổng số mipmap level cần tạo.
	 */
	void GenerateMipmaps(VkCommandBuffer& cmdBuffer, VkImage image, uint32_t width, uint32_t height, uint32_t mipLevels);
};
