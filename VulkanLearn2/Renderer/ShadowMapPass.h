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

/**
 * @struct GeometryPassCreateInfo
 * @brief Cấu trúc chứa tất cả thông tin cần thiết để khởi tạo một GeometryPass.
 */
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

struct ShadowMapHandles
{
	VulkanPipeline* pipeline;
	std::vector<VulkanDescriptor*> descriptors;
};


class ShadowMapPass : public IRenderPass
{
public:
	ShadowMapPass(const ShadowMapPassCreateInfo& shadowInfo);
	~ShadowMapPass();

	const ShadowMapHandles& GetHandles() const { return m_Handles; }
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

	void CreatePipeline(const ShadowMapPassCreateInfo& shadowMapInfo);
	void DrawSceneObject(VkCommandBuffer cmdBuffer, const GPULight& currentLight);
};
