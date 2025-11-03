#include "pch.h"
#include "VulkanFrameBuffer.h"

VulkanFrameBuffer::VulkanFrameBuffer(const VulkanHandles& vulkanHandles, VkRenderPass vkRenderPass, const FrameBufferCreateInfo& frameBufferInfo):
	m_VulkanHandles(vulkanHandles)
{
	VkFramebufferCreateInfo vkFrameBufferInfo{};
	vkFrameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	vkFrameBufferInfo.width = frameBufferInfo.frameWidth;
	vkFrameBufferInfo.height = frameBufferInfo.frameHeigth;
	vkFrameBufferInfo.attachmentCount = frameBufferInfo.imageCount;
	vkFrameBufferInfo.pAttachments = frameBufferInfo.pVkImageView;
	vkFrameBufferInfo.layers = frameBufferInfo.frameLayer;
	vkFrameBufferInfo.renderPass = vkRenderPass;

	vkCreateFramebuffer(m_VulkanHandles.device, &vkFrameBufferInfo, nullptr, &m_Handles.frameBuffer);
}

VulkanFrameBuffer::~VulkanFrameBuffer()
{
	vkDestroyFramebuffer(m_VulkanHandles.device, m_Handles.frameBuffer, nullptr);
}
