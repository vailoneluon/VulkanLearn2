#include "pch.h"
#include "BrightFilterPass.h"
#include "Core/VulkanSampler.h"
#include "Core/VulkanImage.h"
#include "Core/VulkanSwapchain.h"
#include "Core/VulkanPipeline.h"
#include "Core/VulkanDescriptor.h"

BrightFilterPass::BrightFilterPass(const BrightFilterPassCreateInfo& brightFilterInfo) :
	m_VulkanHandles(brightFilterInfo.vulkanHandles),
	m_SwapchainExtent(brightFilterInfo.vulkanSwapchainHandles->swapChainExtent),
	m_BackgroundColor(brightFilterInfo.BackgroundColor),
	m_OutputImage(brightFilterInfo.outputImage)
{
	CreateDescriptor(*brightFilterInfo.inputTextures, brightFilterInfo.vulkanSampler);
	CreatePipeline(brightFilterInfo);
}

BrightFilterPass::~BrightFilterPass()
{
	delete(m_Handles.pipeline);
}

void BrightFilterPass::Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame)
{
	// --- 1. Chuyển đổi Layout cho Ảnh Đầu Ra ---
	// Chuyển layout của ảnh đầu ra từ UNDEFINED sang COLOR_ATTACHMENT_OPTIMAL để có thể ghi vào.
	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_OutputImage)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		0, 1);

	// --- 2. Thiết lập và Bắt đầu Dynamic Rendering ---
	VkRenderingAttachmentInfo colorAttachment{};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.clearValue.color = m_BackgroundColor;
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.imageView = (*m_OutputImage)[currentFrame]->GetHandles().imageView;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	VkRenderingInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;
	renderingInfo.pDepthAttachment = nullptr;
	renderingInfo.pStencilAttachment = nullptr;
	renderingInfo.layerCount = 1;
	renderingInfo.renderArea.extent = m_SwapchainExtent;
	renderingInfo.renderArea.offset = { 0, 0 };

	vkCmdBeginRendering(*cmdBuffer, &renderingInfo);

	// --- 3. Thực hiện Vẽ ---
	vkCmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Handles.pipeline->getHandles().pipeline);
	BindDescriptors(cmdBuffer, currentFrame);
	DrawQuad(*cmdBuffer);

	vkCmdEndRendering(*cmdBuffer);

	// --- 4. Chuyển đổi Layout sau khi Render ---
	// Chuyển layout của ảnh đầu ra sang SHADER_READ_ONLY_OPTIMAL để nó có thể được dùng làm texture trong các pass tiếp theo (cụ thể là BlurPass).
	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_OutputImage)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		0, 1);
}

/**
 * @brief Tạo descriptor set cho ảnh đầu vào (ảnh scene).
 */
void BrightFilterPass::CreateDescriptor(const std::vector<VulkanImage*>& textureImages, const VulkanSampler* vulkanSampler)
{
	m_TextureDescriptors.resize(textureImages.size());
	for (size_t i = 0; i < textureImages.size(); i++)
	{
		// --- Binding 0: Scene Texture ---
		BindingElementInfo bindingElement{};
		bindingElement.binding = 0;
		bindingElement.descriptorCount = 1;
		bindingElement.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindingElement.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = textureImages[i]->GetHandles().imageView;
		imageInfo.sampler = vulkanSampler->getSampler();

		ImageDescriptorUpdateInfo imageUpdateInfo{};
		imageUpdateInfo.binding = 0;
		imageUpdateInfo.firstArrayElement = 0;
		std::vector<VkDescriptorImageInfo> imageInfos{ imageInfo };
		imageUpdateInfo.imageInfos = imageInfos;

		bindingElement.imageDescriptorUpdateInfoCount = 1;
		bindingElement.pImageDescriptorUpdates = &imageUpdateInfo;

		std::vector<BindingElementInfo> bindingElements = { bindingElement };

		m_TextureDescriptors[i] = new VulkanDescriptor(*m_VulkanHandles, bindingElements, 0); // Set 0
		m_Handles.descriptors.push_back(m_TextureDescriptors[i]);
	}
}

/**
 * @brief Tạo pipeline đồ họa cho BrightFilterPass.
 */
void BrightFilterPass::CreatePipeline(const BrightFilterPassCreateInfo& brightFilterInfo)
{
	VulkanPipelineCreateInfo pipelineInfo{};
	pipelineInfo.descriptors = &m_Handles.descriptors;
	pipelineInfo.msaaSamples = brightFilterInfo.MSAA_SAMPLES;
	pipelineInfo.vulkanHandles = brightFilterInfo.vulkanHandles;
	pipelineInfo.useVertexInput = false;
	pipelineInfo.swapchainHandles = brightFilterInfo.vulkanSwapchainHandles;
	pipelineInfo.fragmentShaderFilePath = brightFilterInfo.fragShaderFilePath;
	pipelineInfo.vertexShaderFilePath = brightFilterInfo.vertShaderFilePath;
	pipelineInfo.depthFormat = VK_FORMAT_UNDEFINED;
	pipelineInfo.stencilFormat = VK_FORMAT_UNDEFINED;
	pipelineInfo.cullingMode = VK_CULL_MODE_NONE;

	std::vector<VkFormat> renderingColorAttachments = { VK_FORMAT_R16G16B16A16_SFLOAT };
	pipelineInfo.renderingColorAttachments = &renderingColorAttachments;

	m_Handles.pipeline = new VulkanPipeline(&pipelineInfo);
}

/**
 * @brief Bind descriptor set của frame hiện tại vào command buffer.
 */
void BrightFilterPass::BindDescriptors(const VkCommandBuffer* cmdBuffer, uint32_t currentFrame)
{
	vkCmdBindDescriptorSets(
		*cmdBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_Handles.pipeline->getHandles().pipelineLayout,
		m_TextureDescriptors[currentFrame]->getSetIndex(), 1,
		&m_TextureDescriptors[currentFrame]->getHandles().descriptorSet,
		0, nullptr
	);
}

/**
 * @brief Ghi lệnh vẽ một quad toàn màn hình.
 */
void BrightFilterPass::DrawQuad(VkCommandBuffer cmdBuffer)
{
	vkCmdDraw(cmdBuffer, 6, 1, 0, 0);
}
