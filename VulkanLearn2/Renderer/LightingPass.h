#pragma once
#include "IRenderPass.h"

// Forward declarations
struct VulkanHandles;
struct SwapchainHandles;
class VulkanPipeline;
class VulkanImage;
class VulkanDescriptor;
class VulkanSampler;
class VulkanBuffer;

/**
 * @struct LightingPassCreateInfo
 * @brief Cấu trúc chứa tất cả thông tin cần thiết để khởi tạo một LightingPass.
 */
struct LightingPassCreateInfo
{
	const VulkanHandles* vulkanHandles;
	const SwapchainHandles* vulkanSwapchainHandles;
	const VulkanSampler* vulkanSampler;
	VkSampleCountFlagBits MSAA_SAMPLES;
	uint32_t MAX_FRAMES_IN_FLIGHT;

	// G-Buffer Inputs
	const std::vector<VulkanImage*>* gAlbedoTextures;
	const std::vector<VulkanImage*>* gNormalTextures;
	const std::vector<VulkanImage*>* gPositionTextures;

	// Lighting Inputs
	const std::vector<VulkanDescriptor*>* sceneLightDescriptors;
	const std::vector<VulkanBuffer*>* uniformBuffers; // Camera UBO for viewPos

	std::string fragShaderFilePath;
	std::string vertShaderFilePath;

	VkClearColorValue BackgroundColor;
	const std::vector<VulkanImage*>* outputImages;   // Ảnh đầu ra đã được chiếu sáng.
};

/**
 * @struct LightingPassHandles
 * @brief Chứa các handle nội bộ được quản lý bởi LightingPass.
 */
struct LightingPassHandles
{
	VulkanPipeline* pipeline;
	std::vector<VulkanDescriptor*> descriptors;
};

/**
 * @class LightingPass
 * @brief Thực hiện tính toán ánh sáng dựa trên dữ liệu từ G-Buffer.
 *
 * Pass này nhận các aauh G-Buffer (Albedo, Normal, Position) làm đầu vào,
 * thực hiện các phép tính ánh sáng trong fragment shader và ghi kết quả đã được chiếu sáng
 * vào một ảnh đầu ra.
 */
class LightingPass : public IRenderPass
{
public:
	LightingPass(const LightingPassCreateInfo& lightingInfo);
	~LightingPass();

	void Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame) override;
	const LightingPassHandles& GetHandles() const { return m_Handles; }

private:
	LightingPassHandles m_Handles;

	// --- Tham chiếu đến các tài nguyên bên ngoài ---
	const VulkanHandles* m_VulkanHandles;
	VkExtent2D m_SwapchainExtent;
	VkClearColorValue m_BackgroundColor;

	// --- Tài nguyên dành riêng cho pass ---
	std::vector<VulkanDescriptor*> m_TextureDescriptors; // Descriptors cho G-Buffer đầu vào.
	std::vector<VulkanDescriptor*> m_SceneLightDescriptors; // Descriptors cho Lighting đầu vào.
	std::vector<VulkanDescriptor*> m_UboDescriptors;      // Descriptors cho Camera UBO.
	const std::vector<VulkanImage*>* m_OutputImages;      // Ảnh đầu ra.

	// --- Hàm khởi tạo ---
	void CreateDescriptor(
		const std::vector<VulkanImage*>& gAlbedoTextures,
		const std::vector<VulkanImage*>& gNormalTextures,
		const std::vector<VulkanImage*>& gPositionTextures,
		const VulkanSampler* vulkanSampler,
		const std::vector<VulkanBuffer*>& uniformBuffers
	);
	void CreatePipeline(const LightingPassCreateInfo& lightingInfo);

	// --- Hàm thực thi ---
	void BindDescriptors(const VkCommandBuffer* cmdBuffer, uint32_t currentFrame);
	void DrawQuad(VkCommandBuffer cmdBuffer);
};
