#pragma once
#include "IRenderPass.h"
#include "Core\VulkanContext.h"
#include "Core\VulkanPipeline.h"
#include "Core\VulkanSwapchain.h"
#include "Scene\TextureManager.h"
#include "Core\VulkanImage.h"
#include "Scene\MeshManager.h"
#include "Scene\RenderObject.h"
#include "Core\VulkanDescriptor.h"

struct BrightFilterPassCreateInfo
{
	const VulkanHandles* vulkanHandles;
	const SwapchainHandles* vulkanSwapchainHandles;
	VkSampleCountFlagBits MSAA_SAMPLES;
	uint32_t MAX_FRAMES_IN_FLIGHT;

	const std::vector<VulkanDescriptor*>* textureDescriptors;

	std::string fragShaderFilePath;
	std::string vertShaderFilePath;

	VkClearColorValue BackgroundColor;
	const std::vector<VulkanImage*>* outputImage;
};

struct BrightFilterPassHandles
{
	VulkanPipeline* pipeline;
	std::vector<VulkanDescriptor*> descriptors;
};

class BrightFilterPass : public IRenderPass
{
public:
	BrightFilterPass(const BrightFilterPassCreateInfo& brightFilterInfo);
	~BrightFilterPass();
	const BrightFilterPassHandles& GetHandles() const { return m_Handles; }

	void Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame) override;

private:
	BrightFilterPassHandles m_Handles;

	const VulkanHandles* m_VulkanHandles;
	VkExtent2D m_SwapchainExtent;
	VkClearColorValue m_BackgroundColor;
	const std::vector<VulkanDescriptor*>* m_TextureDescriptors;

	const std::vector<VulkanImage*>* m_OutputImage;

	void CreateDescriptor();
	void CreatePipeline(const BrightFilterPassCreateInfo& brightFilterInfo);

	void BindDescriptors(const VkCommandBuffer* cmdBuffer, uint32_t currentFrame);
	void DrawQuad(VkCommandBuffer cmdBuffer);
};
