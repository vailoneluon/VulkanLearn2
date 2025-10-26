#pragma once
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "VulkanSwapchain.h"
#include "VulkanRenderPass.h"

struct FrameBufferHandles 
{
	std::vector<VkFramebuffer> frameBuffers;
	VulkanImage* colorImage;
	VulkanImage* depthStencilImage;
};

class VulkanFrameBuffer
{
public:
	VulkanFrameBuffer(const VulkanHandles& vulkanHandles, const SwapchainHandles& swapchainHandle, const RenderPassHandles& renderPassHandles, const VkSampleCountFlagBits Samples);
	~VulkanFrameBuffer();

	const FrameBufferHandles& getHandles() const { return handles; }

private:
	FrameBufferHandles handles;
	const VulkanHandles& vk;
	const SwapchainHandles& sc;
	const RenderPassHandles& rp;
	const VkSampleCountFlagBits msaaSamples;
	
	void CreateColorResources();
	void CreateDepthStencilResources();
	void CreateFrameBuffers();
};
