#pragma once
#include "IRenderPass.h"

// Forward declarations để giảm phụ thuộc vào header
struct VulkanHandles;
struct SwapchainHandles;
class VulkanPipeline;
class VulkanImage;
class VulkanDescriptor;
class VulkanSampler;

/**
 * @struct BlurPassCreateInfo
 * @brief Cấu trúc chứa tất cả thông tin cần thiết để khởi tạo một BlurPass.
 * Đóng gói các phụ thuộc như Vulkan handles, ảnh đầu vào/đầu ra, và shader.
 */
struct BlurPassCreateInfo
{
	const VulkanHandles* vulkanHandles;
	const SwapchainHandles* vulkanSwapchainHandles;
	VkSampleCountFlagBits MSAA_SAMPLES;

	const std::vector<VulkanImage*>* inputTextures;	// Ảnh đầu vào cần được làm mờ.
	const VulkanSampler* vulkanSampler;				// Sampler để đọc ảnh đầu vào.

	std::string fragShaderFilePath;					// Đường dẫn đến fragment shader (ví dụ: blur_horizontal.frag).
	std::string vertShaderFilePath;					// Đường dẫn đến vertex shader (thường là shader vẽ quad toàn màn hình).

	VkClearColorValue BackgroundColor;
	const std::vector<VulkanImage*>* outputImages;	// Ảnh đầu ra để ghi kết quả đã làm mờ.
};

/**
 * @struct BlurPassHandles
 * @brief Chứa các handle nội bộ được quản lý bởi BlurPass.
 */
struct BlurPassHandles
{
	VulkanPipeline* pipeline;						// Pipeline đồ họa dành riêng cho pass này.
	std::vector<VulkanDescriptor*> descriptors;		// Danh sách các descriptor set (một cho mỗi frame-in-flight).
};

/**
 * @class BlurPass
 * @brief Thực hiện một bước làm mờ Gaussian (theo chiều ngang hoặc dọc) trên một ảnh đầu vào.
 *
 * Pass này vẽ một quad toàn màn hình và áp dụng shader làm mờ. Nó nhận một ảnh đầu vào,
 * đọc từ nó bằng sampler, và ghi kết quả đã làm mờ vào một ảnh đầu ra.
 * Thường được sử dụng hai lần liên tiếp (một lần cho chiều ngang, một lần cho chiều dọc)
 * để tạo hiệu ứng blur hoàn chỉnh.
 */
class BlurPass : public IRenderPass
{
public:
	BlurPass(const BlurPassCreateInfo& blurInfo);
	~BlurPass();

	void Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame) override;
	const BlurPassHandles& GetHandles() const { return m_Handles; }

private:
	BlurPassHandles m_Handles;

	// --- Tham chiếu đến các tài nguyên bên ngoài ---
	const VulkanHandles* m_VulkanHandles;
	VkExtent2D m_SwapchainExtent;
	VkClearColorValue m_BackgroundColor;
	
	// --- Tài nguyên dành riêng cho pass ---
	std::vector<VulkanDescriptor*> m_TextureDescriptors; // Descriptors cho ảnh đầu vào.
	const std::vector<VulkanImage*>* m_OutputImages;     // Ảnh đầu ra.

	// --- Hàm khởi tạo ---
	void CreateDescriptor(const std::vector<VulkanImage*>& inputTextures, const VulkanSampler* vulkanSampler);
	void CreatePipeline(const BlurPassCreateInfo& blurInfo);

	// --- Hàm thực thi ---
	void BindDescriptors(const VkCommandBuffer* cmdBuffer, uint32_t currentFrame);
	void DrawQuad(const VkCommandBuffer* cmdBuffer);
};

