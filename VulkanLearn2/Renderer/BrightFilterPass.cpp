#include "pch.h"
#include "BrightFilterPass.h"

BrightFilterPass::BrightFilterPass(const BrightFilterPassCreateInfo& brightFilterInfo):
	m_VulkanHandles(brightFilterInfo.vulkanHandles),
	m_SwapchainExtent(brightFilterInfo.vulkanSwapchainHandles->swapChainExtent),
	m_BackgroundColor(brightFilterInfo.BackgroundColor),
	m_TextureDescriptors(brightFilterInfo.textureDescriptors),
	m_OutputImage(brightFilterInfo.outputImage)
{
	CreateDescriptor();
	CreatePipeline(brightFilterInfo);
}

BrightFilterPass::~BrightFilterPass()
{
	delete(m_Handles.pipeline);
}

void BrightFilterPass::Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame)
{
	// Transition Layout trước khi BeginRendering vì không còn Dependency quản lý.
	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_OutputImage)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		0, 0);

	VkRenderingAttachmentInfo colorAttachment{};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.clearValue.color = m_BackgroundColor;
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.imageView = (*m_OutputImage)[currentFrame]->GetHandles().imageView;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	// Rendering Info dùng để bắt đầu dynamic rendering - thay thế vkcmdBeginRenderPass
	VkRenderingInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;
	renderingInfo.pDepthAttachment = nullptr;
	renderingInfo.pStencilAttachment = nullptr;
	renderingInfo.layerCount = 1;
	renderingInfo.renderArea.extent = m_SwapchainExtent;
	renderingInfo.renderArea.offset = { 0, 0 };
	renderingInfo.viewMask = 0;

	vkCmdBeginRendering(*cmdBuffer, &renderingInfo);

	vkCmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Handles.pipeline->getHandles().pipeline);
	BindDescriptors(cmdBuffer, currentFrame);
	DrawQuad(*cmdBuffer);

	vkCmdEndRendering(*cmdBuffer);

	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_OutputImage)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		0, 0);
}

void BrightFilterPass::CreateDescriptor()
{
	m_Handles.descriptors.insert(m_Handles.descriptors.end(), m_TextureDescriptors->begin(), m_TextureDescriptors->end());
}

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

	m_Handles.pipeline = new VulkanPipeline(&pipelineInfo);
}

void BrightFilterPass::BindDescriptors(const VkCommandBuffer* cmdBuffer, uint32_t currentFrame)
{
	vkCmdBindDescriptorSets(
		*cmdBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS, m_Handles.pipeline->getHandles().pipelineLayout,
		(*m_TextureDescriptors)[currentFrame]->getSetIndex(), 1,
		&(*m_TextureDescriptors)[currentFrame]->getHandles().descriptorSet,
		0, nullptr
	);
}

void BrightFilterPass::DrawQuad(VkCommandBuffer cmdBuffer)
{
	vkCmdDraw(cmdBuffer, 6, 1, 0, 0);
}
