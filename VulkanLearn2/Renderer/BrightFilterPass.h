#pragma once
#include "IRenderPass.h"

// Forward declarations
struct VulkanHandles;
struct SwapchainHandles;
class VulkanPipeline;
class VulkanImage;
class VulkanDescriptor;
class VulkanSampler;

// =================================================================================================
// Struct: BrightFilterPassCreateInfo
// Mô tả: Cấu trúc chứa tất cả thông tin cần thiết để khởi tạo một BrightFilterPass.
// =================================================================================================
struct BrightFilterPassCreateInfo
{
	const VulkanHandles* vulkanHandles;
	const SwapchainHandles* vulkanSwapchainHandles;
	const VulkanSampler* vulkanSampler;
	VkSampleCountFlagBits MSAA_SAMPLES;
	uint32_t MAX_FRAMES_IN_FLIGHT;

	const std::vector<VulkanImage*>* inputTextures; // Ảnh đầu vào (thường là ảnh scene đã render).

	std::string fragShaderFilePath;
	std::string vertShaderFilePath;

	VkClearColorValue BackgroundColor;
	const std::vector<VulkanImage*>* outputImage;   // Ảnh đầu ra chỉ chứa các vùng sáng.
};

// =================================================================================================
// Struct: BrightFilterPassHandles
// Mô tả: Chứa các handle nội bộ được quản lý bởi BrightFilterPass.
// =================================================================================================
struct BrightFilterPassHandles
{
	VulkanPipeline* pipeline;
	std::vector<VulkanDescriptor*> descriptors;
};

// =================================================================================================
// Class: BrightFilterPass
// Mô tả: 
//      Lọc ra các vùng có độ sáng vượt ngưỡng từ một ảnh đầu vào.
//      Đây là bước đầu tiên của hiệu ứng bloom. Pass này nhận ảnh đã render của scene,
//      và tạo ra một ảnh mới chỉ chứa các pixel có độ sáng cao hơn một giá trị nhất định (threshold).
//      Kết quả của pass này sẽ được dùng làm đầu vào cho các bước blur.
// =================================================================================================
class BrightFilterPass : public IRenderPass
{
public:
	// Constructor: Khởi tạo BrightFilterPass với các thông tin cấu hình.
	BrightFilterPass(const BrightFilterPassCreateInfo& brightFilterInfo);
	~BrightFilterPass();

	// Thực thi pass render.
	void Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame) override;
	
	// Getter: Lấy các handle nội bộ.
	const BrightFilterPassHandles& GetHandles() const { return m_Handles; }

private:
	BrightFilterPassHandles m_Handles;

	// --- Tham chiếu đến các tài nguyên bên ngoài ---
	const VulkanHandles* m_VulkanHandles;
	VkExtent2D m_SwapchainExtent;
	VkClearColorValue m_BackgroundColor;

	// --- Tài nguyên dành riêng cho pass ---
	std::vector<VulkanDescriptor*> m_TextureDescriptors; // Descriptors cho ảnh đầu vào.
	const std::vector<VulkanImage*>* m_OutputImage;      // Ảnh đầu ra.

	// --- Hàm khởi tạo ---
	
	// Helper: Tạo descriptor sets.
	void CreateDescriptor(const std::vector<VulkanImage*>& textureImages, const VulkanSampler* vulkanSampler);
	
	// Helper: Tạo pipeline đồ họa.
	void CreatePipeline(const BrightFilterPassCreateInfo& brightFilterInfo);

	// --- Hàm thực thi ---
	
	// Helper: Bind các descriptor set trước khi vẽ.
	void BindDescriptors(const VkCommandBuffer* cmdBuffer, uint32_t currentFrame);
	
	// Helper: Vẽ một hình chữ nhật full-screen để chạy fragment shader.
	void DrawQuad(VkCommandBuffer cmdBuffer);
};
