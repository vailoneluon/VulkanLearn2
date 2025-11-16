#pragma once
#include "IRenderPass.h"

// Forward declarations
struct VulkanHandles;
struct SwapchainHandles;
class VulkanPipeline;
class VulkanImage;
class VulkanDescriptor;
class VulkanSampler;

/**
 * @struct CompositePassCreateInfo
 * @brief Cấu trúc chứa tất cả thông tin cần thiết để khởi tạo một CompositePass.
 */
struct CompositePassCreateInfo
{
	const VulkanHandles* vulkanHandles;
	const SwapchainHandles* vulkanSwapchainHandles;
	VkSampleCountFlagBits MSAA_SAMPLES;

	const std::vector<VulkanImage*>* inputTextures0; // Ảnh đầu vào 1 (thường là ảnh scene gốc).
	const std::vector<VulkanImage*>* inputTextures1; // Ảnh đầu vào 2 (thường là ảnh bloom đã qua xử lý).
	const VulkanSampler* vulkanSampler;

	std::string fragShaderFilePath;
	std::string vertShaderFilePath;

	VkClearColorValue BackgroundColor;
	VulkanImage* mainColorImage;       // Attachment màu (MSAA) để vẽ.
	VulkanImage* mainDepthStencilImage; // Attachment depth/stencil.
};

/**
 * @struct CompositePassHandles
 * @brief Chứa các handle nội bộ được quản lý bởi CompositePass.
 */
struct CompositePassHandles
{
	VulkanPipeline* pipeline;
	std::vector<VulkanDescriptor*> descriptors;
};

/**
 * @class CompositePass
 * @brief Tổng hợp (composite) ảnh scene gốc và ảnh hiệu ứng (bloom) để tạo ra ảnh cuối cùng.
 *
 * Đây là bước cuối cùng trong chuỗi render. Pass này nhận hai ảnh đầu vào,
 * cộng chúng lại với nhau trong fragment shader, và vẽ kết quả vào một attachment màu (MSAA).
 * Sau đó, kết quả này được resolve vào swapchain image, sẵn sàng để được trình chiếu lên màn hình.
 */
class CompositePass : public IRenderPass
{
public:
	CompositePass(const CompositePassCreateInfo& compositeInfo);
	~CompositePass();

	void Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame) override;
	const CompositePassHandles& GetHandles() const { return m_Handles; }

private:
	CompositePassHandles m_Handles;

	// --- Tham chiếu đến các tài nguyên bên ngoài ---
	const VulkanHandles* m_VulkanHandles;
	const SwapchainHandles* m_VulkanSwapchainHandles;
	VkExtent2D m_SwapchainExtent;
	VkClearColorValue m_BackgroundColor;

	// --- Tài nguyên dành riêng cho pass ---
	std::vector<VulkanDescriptor*> m_TextureDescriptors; // Descriptors cho 2 ảnh đầu vào.
	VulkanImage* m_MainColorImage;
	VulkanImage* m_MainDepthStencilImage;

	// --- Hàm khởi tạo ---
	void CreateDescriptor(const std::vector<VulkanImage*>& inputTextures0, const std::vector<VulkanImage*>& inputTextures1, const VulkanSampler* vulkanSampler);
	void CreatePipeline(const CompositePassCreateInfo& compositeInfo);

	// --- Hàm thực thi ---
	void BindDescriptors(const VkCommandBuffer* cmdBuffer, uint32_t currentFrame);
	void DrawQuad(const VkCommandBuffer* cmdBuffer);
};
