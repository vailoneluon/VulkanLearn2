#pragma once
#include "IRenderPass.h"

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
struct GeometryPassCreateInfo
{
	const VulkanHandles* vulkanHandles;
	const SwapchainHandles* vulkanSwapchainHandles;
	VkSampleCountFlagBits MSAA_SAMPLES;
	uint32_t MAX_FRAMES_IN_FLIGHT;

	// --- Dữ liệu Scene ---
	const TextureManager* textureManager;
	const MeshManager* meshManager;
	MaterialManager* materialManager;
	const std::vector<RenderObject*>* renderObjects;
	const std::vector<VulkanBuffer*>* uniformBuffers; // UBO chứa ma trận camera.

	// --- Shaders ---
	std::string fragShaderFilePath;
	std::string vertShaderFilePath;

	// --- Attachments & Đầu ra ---
	VkClearColorValue BackgroundColor;
	const std::vector<VulkanImage*>* colorImages;        // Attachment màu (MSAA) để vẽ.
	const std::vector<VulkanImage*>* depthStencilImages; // Attachment depth/stencil (MSAA).
	const std::vector<VulkanImage*>* outputImage;        // Ảnh đầu ra (đã resolve) để dùng trong post-processing.

	// --- Dữ liệu Shader ---
	PushConstantData* pushConstantData;
};

/**
 * @struct GeometryPassHandles
 * @brief Chứa các handle nội bộ được quản lý bởi GeometryPass.
 */
struct GeometryPassHandles
{
	VulkanPipeline* pipeline;
	std::vector<VulkanDescriptor*> descriptors;
};

/**
 * @class GeometryPass
 * @brief Thực hiện bước render chính của scene 3D (Render-To-Texture - RTT).
 *
 * Pass này chịu trách nhiệm vẽ tất cả các đối tượng hình học (RenderObject) trong scene.
 * Nó sử dụng MSAA để khử răng cưa, vẽ vào một cặp attachment màu và depth (MSAA),
 * sau đó resolve kết quả vào một ảnh không-MSAA. Ảnh này sẽ trở thành đầu vào
 * cho chuỗi post-processing.
 */
class GeometryPass : public IRenderPass
{
public:
	GeometryPass(const GeometryPassCreateInfo& geometryInfo);
	~GeometryPass();

	const GeometryPassHandles& GetHandles() const { return m_Handles; }
	void Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame) override;

private:
	GeometryPassHandles m_Handles;

	// --- Tham chiếu đến các tài nguyên bên ngoài ---
	const MeshManager* m_MeshManager;
	MaterialManager* m_MaterialManager;
	const VulkanHandles* m_VulkanHandles;
	PushConstantData* m_PushConstantData;
	const std::vector<RenderObject*>* m_RenderObjects;
	VkExtent2D m_SwapchainExtent;
	VkClearColorValue m_BackgroundColor;

	// --- Tài nguyên dành riêng cho pass ---
	VulkanDescriptor* m_TextureDescriptors;				// Descriptor cho mảng texture (Set 0).
	std::vector<VulkanDescriptor*> m_UboDescriptors;	// Descriptors cho UBO camera (Set 1), một cho mỗi frame.
	const std::vector<VulkanImage*>* m_ColorImages;
	const std::vector<VulkanImage*>* m_DepthStencilImages;
	const std::vector<VulkanImage*>* m_OutputImage;

	// --- Hàm khởi tạo ---
	void CreateDescriptor(const std::vector<VulkanBuffer*>& uniformBuffers);
	void CreatePipeline(const GeometryPassCreateInfo& geometryInfo);

	// --- Hàm thực thi ---
	void BindDescriptors(const VkCommandBuffer* cmdBuffer, uint32_t currentFrame);
	void DrawSceneObject(VkCommandBuffer cmdBuffer);
};
