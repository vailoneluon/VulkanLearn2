#pragma once
#include "IRenderPass.h"
#include "Scene\LightManager.h"

// Forward declarations
struct VulkanHandles;
struct SwapchainHandles;
struct PushConstantData;
class VulkanPipeline;
class VulkanImage;
class VulkanDescriptor;
class VulkanBuffer;
class TextureManager;
class MeshManager;
class RenderObject;
class MaterialManager;

// =================================================================================================
// Struct: ShadowMapPassCreateInfo
// Mô tả: Cấu trúc chứa tất cả thông tin cần thiết để khởi tạo một ShadowMapPass.
// =================================================================================================
struct ShadowMapPassCreateInfo
{
	const VulkanHandles* vulkanHandles;
	const SwapchainHandles* vulkanSwapchainHandles;
	VkSampleCountFlagBits MSAA_SAMPLES;
	uint32_t MAX_FRAMES_IN_FLIGHT;

	// --- Dữ liệu Scene ---
	const MeshManager* meshManager;
	const LightManager* lightManager;
	const std::vector<RenderObject*>* renderObjects;

	// --- Shaders ---
	std::string fragShaderFilePath;
	std::string vertShaderFilePath;

	VkClearColorValue BackgroundColor;
};

// =================================================================================================
// Struct: ShadowMapHandles
// Mô tả: Chứa các handle nội bộ được quản lý bởi ShadowMapPass.
// =================================================================================================
struct ShadowMapHandles
{
	VulkanPipeline* pipeline;
	std::vector<VulkanDescriptor*> descriptors;
};

// =================================================================================================
// Class: ShadowMapPass
// Mô tả: 
//      Thực hiện render scene từ góc nhìn của đèn để tạo shadow map.
//      Pass này sẽ render độ sâu của scene vào một depth texture (shadow map).
//      Dữ liệu này sau đó được dùng trong LightingPass để tính toán bóng đổ.
// =================================================================================================
class ShadowMapPass : public IRenderPass
{
public:
	// Constructor: Khởi tạo ShadowMapPass với các thông tin cấu hình.
	ShadowMapPass(const ShadowMapPassCreateInfo& shadowInfo);
	~ShadowMapPass();

	// Getter: Lấy các handle nội bộ.
	const ShadowMapHandles& GetHandles() const { return m_Handles; }
	
	// Thực thi pass render.
	void Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame) override;

private:
	ShadowMapHandles m_Handles;

	ShadowMapPushConstantData m_PushConstantData;

	// --- Tham chiếu đến các tài nguyên bên ngoài ---
	const MeshManager* m_MeshManager;
	const VulkanHandles* m_VulkanHandles;
	const LightManager* m_LightManager;
	const std::vector<RenderObject*>* m_RenderObjects;
	VkClearColorValue m_BackgroundColor;

	// --- Hàm khởi tạo ---
	
	// Helper: Tạo pipeline đồ họa.
	void CreatePipeline(const ShadowMapPassCreateInfo& shadowMapInfo);
	
	// --- Hàm thực thi ---
	
	// Helper: Vẽ các đối tượng trong scene.
	void DrawSceneObject(VkCommandBuffer cmdBuffer, const GPULight& currentLight);
};
