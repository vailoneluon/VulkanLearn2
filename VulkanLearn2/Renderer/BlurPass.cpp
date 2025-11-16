#include "pch.h"
#include "BlurPass.h"

BlurPass::BlurPass(const BlurPassCreateInfo& blurInfo) :
	m_VulkanHandles(blurInfo.vulkanHandles),
	m_SwapchainExtent(blurInfo.vulkanSwapchainHandles->swapChainExtent),
	m_BackgroundColor(blurInfo.BackgroundColor),
	m_InputTextureDescriptors(blurInfo.inputTextureDescriptors),
	m_OutputImages(blurInfo.outputImages)
{
	CreateDescriptor();
	CreatePipeline(blurInfo);
}

BlurPass::~BlurPass()
{
	delete(m_Handles.pipeline);
}

void BlurPass::Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame)
{
	// Transition Layout trước khi BeginRendering vì không còn Dependency quản lý.
	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_OutputImages)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		0, 0);

	VkRenderingAttachmentInfo colorAttachment{};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.clearValue.color = m_BackgroundColor;
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.imageView = (*m_OutputImages)[currentFrame]->GetHandles().imageView;
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
	DrawQuad(cmdBuffer);

	vkCmdEndRendering(*cmdBuffer);

	VulkanImage::TransitionLayout(
		*cmdBuffer, (*m_OutputImages)[currentFrame]->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		0, 0);
}

void BlurPass::CreateDescriptor()
{
	m_Handles.descriptors.insert(m_Handles.descriptors.end(), m_InputTextureDescriptors->begin(), m_InputTextureDescriptors->end());
}

void BlurPass::CreatePipeline(const BlurPassCreateInfo& blurInfo)
{
	VulkanPipelineCreateInfo pipelineInfo{};
	pipelineInfo.descriptors = &m_Handles.descriptors;
	pipelineInfo.msaaSamples = blurInfo.MSAA_SAMPLES;
	pipelineInfo.vulkanHandles = blurInfo.vulkanHandles;
	pipelineInfo.useVertexInput = false;
	pipelineInfo.swapchainHandles = blurInfo.vulkanSwapchainHandles;
	pipelineInfo.fragmentShaderFilePath = blurInfo.fragShaderFilePath;
	pipelineInfo.vertexShaderFilePath = blurInfo.vertShaderFilePath;

	m_Handles.pipeline = new VulkanPipeline(&pipelineInfo);
}

void BlurPass::BindDescriptors(const VkCommandBuffer* cmdBuffer, uint32_t currentFrame)
{
	vkCmdBindDescriptorSets(
		*cmdBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS, m_Handles.pipeline->getHandles().pipelineLayout,
		(*m_InputTextureDescriptors)[currentFrame]->getSetIndex(), 1,
		&(*m_InputTextureDescriptors)[currentFrame]->getHandles().descriptorSet,
		0, nullptr
	);
}

void BlurPass::DrawQuad(const VkCommandBuffer* cmdBuffer)
{
	vkCmdDraw(*cmdBuffer, 6, 1, 0, 0);
}
