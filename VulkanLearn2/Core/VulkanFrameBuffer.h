#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanImage.h"
#include "VulkanRenderPass.h"
using namespace std;

struct FrameBufferHandles 
{
	vector<VkFramebuffer> frameBuffers;
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
