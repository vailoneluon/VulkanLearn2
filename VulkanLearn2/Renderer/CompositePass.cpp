#include "pch.h"
#include "CompositePass.h"

CompositePass::CompositePass(const CompositePassCreateInfo& compositeInfo) :
	m_VulkanHandles(compositeInfo.vulkanHandles),
	m_VulkanSwapchainHandles(compositeInfo.vulkanSwapchainHandles),
	m_SwapchainExtent(compositeInfo.vulkanSwapchainHandles->swapChainExtent),
	m_BackgroundColor(compositeInfo.BackgroundColor),
	m_TextureDescriptors(compositeInfo.textureDescriptors),
	m_MainColorImage(compositeInfo.mainColorImage),
	m_MainDepthStencilImage(compositeInfo.mainDepthStencilImage)
{
	CreateDescriptor();
	CreatePipeline(compositeInfo);
}

CompositePass::~CompositePass()
{
	delete(m_Handles.pipeline);
}

void CompositePass::Execute(const VkCommandBuffer* cmdBuffer, uint32_t imageIndex, uint32_t currentFrame)
{
	// Transition Layout trước khi BeginRendering vì không còn Dependency quản lý.
	VulkanImage::TransitionLayout(
		*cmdBuffer, m_MainColorImage->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		0, 0);

	VulkanImage::TransitionLayout(
		*cmdBuffer, m_MainDepthStencilImage->GetHandles().image, 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		0, 0);

	VulkanImage::TransitionLayout(
		*cmdBuffer, m_VulkanSwapchainHandles->swapchainImages[imageIndex], 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		0, 0);

	VkRenderingAttachmentInfo colorAttachment{};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.clearValue.color = m_BackgroundColor;
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.imageView = m_MainColorImage->GetHandles().imageView;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// Resolve Msaa 
	colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.resolveImageView = m_VulkanSwapchainHandles->swapchainImageViews[imageIndex];
	colorAttachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;

	VkRenderingAttachmentInfo depthStencilAttachment{};
	depthStencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depthStencilAttachment.clearValue.depthStencil = { 1.0f, 0 };
	depthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthStencilAttachment.imageView = m_MainDepthStencilImage->GetHandles().imageView;
	depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Rendering Info dùng để bắt đầu dynamic rendering - thay thế vkcmdBeginRenderPass
	VkRenderingInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;
	renderingInfo.pDepthAttachment = &depthStencilAttachment;
	renderingInfo.pStencilAttachment = &depthStencilAttachment;
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
		*cmdBuffer, m_VulkanSwapchainHandles->swapchainImages[imageIndex], 1,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0,
		0, 0);
}

void CompositePass::CreateDescriptor()
{
	m_Handles.descriptors.insert(m_Handles.descriptors.end(), m_TextureDescriptors->begin(), m_TextureDescriptors->end());
}

void CompositePass::CreatePipeline(const CompositePassCreateInfo& compositeInfo)
{
	VulkanPipelineCreateInfo pipelineInfo{};
	pipelineInfo.descriptors = &m_Handles.descriptors;
	pipelineInfo.msaaSamples = compositeInfo.MSAA_SAMPLES;
	pipelineInfo.vulkanHandles = compositeInfo.vulkanHandles;
	pipelineInfo.useVertexInput = false;
	pipelineInfo.swapchainHandles = compositeInfo.vulkanSwapchainHandles;
	pipelineInfo.fragmentShaderFilePath = compositeInfo.fragShaderFilePath;
	pipelineInfo.vertexShaderFilePath = compositeInfo.vertShaderFilePath;

	m_Handles.pipeline = new VulkanPipeline(&pipelineInfo);
}

void CompositePass::BindDescriptors(const VkCommandBuffer* cmdBuffer, uint32_t currentFrame)
{
	vkCmdBindDescriptorSets(
		*cmdBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS, m_Handles.pipeline->getHandles().pipelineLayout,
		(*m_TextureDescriptors)[currentFrame]->getSetIndex(), 1,
		&(*m_TextureDescriptors)[currentFrame]->getHandles().descriptorSet,
		0, nullptr
	);
}

void CompositePass::DrawQuad(const VkCommandBuffer* cmdBuffer)
{
	vkCmdDraw(*cmdBuffer, 6, 1, 0, 0);
}
