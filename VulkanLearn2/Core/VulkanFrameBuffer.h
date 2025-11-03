#pragma once
#include "VulkanContext.h"

struct FrameBufferHandles
{
	VkFramebuffer frameBuffer;
};

struct FrameBufferCreateInfo
{
	uint32_t imageCount;
	VkImageView* pVkImageView;
	
	uint32_t frameWidth;
	uint32_t frameHeigth;
	uint32_t frameLayer;
};

class VulkanFrameBuffer
{
public:
	VulkanFrameBuffer(const VulkanHandles& vulkanHandles, VkRenderPass vkRenderPass, const FrameBufferCreateInfo& frameBufferInfo);
	~VulkanFrameBuffer();

	VkFramebuffer getFrameBuffer() { return m_Handles.frameBuffer; }

private:
	FrameBufferHandles m_Handles;
	
	const VulkanHandles& m_VulkanHandles;
};
