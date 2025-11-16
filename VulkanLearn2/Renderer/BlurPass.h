#pragma once
#include "IRenderPass.h"
#include "Core/VulkanContext.h"
#include "Core/VulkanPipeline.h"
#include "Core/VulkanSwapchain.h"
#include "Core/VulkanImage.h"
#include "Core/VulkanDescriptor.h"

struct BlurPassCreateInfo
{
	const VulkanHandles* vulkanHandles;
	const SwapchainHandles* vulkanSwapchainHandles;
	VkSampleCountFlagBits MSAA_SAMPLES;

	const std::vector<VulkanDescriptor*>* inputTextureDescriptors; // Descriptor for the texture to be blurred

	std::string fragShaderFilePath;
	std::string vertShaderFilePath;

	VkClearColorValue BackgroundColor;
	const std::vector<VulkanImage*>* outputImages; // Image where the blurred result is written
};

struct BlurPassHandles
{
	VulkanPipeline* pipeline;
	std::vector<VulkanDescriptor*> descriptors;
};

class BlurPass : public IRenderPass
{
public:
	BlurPass(const BlurPassCreateInfo& blurInfo);
	~BlurPass();

	void Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame) override;
	const BlurPassHandles& GetHandles() const { return m_Handles; }

private:
	BlurPassHandles m_Handles;

	const VulkanHandles* m_VulkanHandles;
	VkExtent2D m_SwapchainExtent;
	VkClearColorValue m_BackgroundColor;
	
	const std::vector<VulkanDescriptor*>* m_InputTextureDescriptors;
	const std::vector<VulkanImage*>* m_OutputImages;

	void CreateDescriptor();
	void CreatePipeline(const BlurPassCreateInfo& blurInfo);

	void BindDescriptors(const VkCommandBuffer* cmdBuffer, uint32_t currentFrame);
	void DrawQuad(const VkCommandBuffer* cmdBuffer);
};
