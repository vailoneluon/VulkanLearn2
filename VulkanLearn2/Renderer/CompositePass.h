#pragma once
#include "IRenderPass.h"
#include "Core/VulkanContext.h"
#include "Core/VulkanPipeline.h"
#include "Core/VulkanSwapchain.h"
#include "Core/VulkanImage.h"
#include "Core/VulkanDescriptor.h"

struct CompositePassCreateInfo
{
	const VulkanHandles* vulkanHandles;
	const SwapchainHandles* vulkanSwapchainHandles;
	VkSampleCountFlagBits MSAA_SAMPLES;

	const std::vector<VulkanDescriptor*>* textureDescriptors;

	std::string fragShaderFilePath;
	std::string vertShaderFilePath;

	VkClearColorValue BackgroundColor;
	VulkanImage* mainColorImage;
	VulkanImage* mainDepthStencilImage;
};

struct CompositePassHandles
{
	VulkanPipeline* pipeline;
	std::vector<VulkanDescriptor*> descriptors;
};

class CompositePass : public IRenderPass
{
public:
	CompositePass(const CompositePassCreateInfo& compositeInfo);
	~CompositePass();

	void Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame) override;
	const CompositePassHandles& GetHandles() const { return m_Handles; }

private:
	CompositePassHandles m_Handles;

	const VulkanHandles* m_VulkanHandles;
	const SwapchainHandles* m_VulkanSwapchainHandles;
	VkExtent2D m_SwapchainExtent;
	VkClearColorValue m_BackgroundColor;
	
	const std::vector<VulkanDescriptor*>* m_TextureDescriptors;
	VulkanImage* m_MainColorImage;
	VulkanImage* m_MainDepthStencilImage;

	void CreateDescriptor();
	void CreatePipeline(const CompositePassCreateInfo& compositeInfo);

	void BindDescriptors(const VkCommandBuffer* cmdBuffer, uint32_t currentFrame);
	void DrawQuad(const VkCommandBuffer* cmdBuffer);
};
