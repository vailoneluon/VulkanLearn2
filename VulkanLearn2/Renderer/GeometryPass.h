#pragma once
#include "IRenderPass.h"
#include "Core\VulkanContext.h"
#include "Core\VulkanPipeline.h"
#include "Core\VulkanSwapchain.h"
#include "Scene\TextureManager.h"
#include "Core\VulkanImage.h"
#include "Scene\MeshManager.h"
#include "Scene\RenderObject.h"

struct GeometryPassCreateInfo
{
	const VulkanHandles* vulkanHandles;
	const SwapchainHandles* vulkanSwapchainHandles;
	VkSampleCountFlagBits MSAA_SAMPLES;
	uint32_t MAX_FRAMES_IN_FLIGHT;
	
	const TextureManager* textureManager;
	const MeshManager* meshManager;
	const std::vector<RenderObject*>* renderObjects;
	const std::vector<VulkanDescriptor*>* uboDescriptors;

	std::string fragShaderFilePath;
	std::string vertShaderFilePath;

	VkClearColorValue BackgroundColor;
	const std::vector<VulkanImage*>* colorImages;
	const std::vector<VulkanImage*>* depthStencilImages;
	const std::vector<VulkanImage*>* outputImage;
	
	PushConstantData* pushConstantData;
	
};

struct GeometryPassHandles
{
	VulkanPipeline* pipeline;
	std::vector<VulkanDescriptor*> descriptors;
};

class GeometryPass : public IRenderPass
{
public:
	GeometryPass(const GeometryPassCreateInfo& geometryInfo);
	~GeometryPass();

	const GeometryPassHandles& GetHandles() const { return m_Handles; }

	void Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame) override;

private:
	GeometryPassHandles m_Handles;
	
	const MeshManager* m_MeshManager;
	const VulkanHandles* m_VulkanHandles;
	PushConstantData* m_PushConstantData;
	const std::vector<RenderObject*>* m_RenderObjects;
	VkExtent2D m_SwapchainExtent;
	VkClearColorValue m_BackgroundColor;
	VulkanDescriptor* m_TextureDescriptors;
	const std::vector<VulkanDescriptor*>* m_UboDescriptors;
	const std::vector<VulkanImage*>* m_ColorImages;
	const std::vector<VulkanImage*>* m_DepthStencilImages;
	const std::vector<VulkanImage*>* m_OutputImage;
	

	void CreateDescriptor();
	void CreatePipeline(const GeometryPassCreateInfo& geometryInfo);

	void BindDescriptors(const VkCommandBuffer* cmdBuffer, uint32_t currentFrame);
	void DrawSceneObject(VkCommandBuffer cmdBuffer);
};
