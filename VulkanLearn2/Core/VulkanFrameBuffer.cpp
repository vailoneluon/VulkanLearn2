#include "pch.h"
#include "VulkanFrameBuffer.h"


VulkanFrameBuffer::VulkanFrameBuffer(const VulkanHandles& vulkanHandles, const SwapchainHandles& swapchainHandle, const RenderPassHandles& renderPassHandles, const VkSampleCountFlagBits Samples):
	vk(vulkanHandles), sc(swapchainHandle), msaaSamples(Samples), rp(renderPassHandles)
{
	CreateColorResources();
	CreateDepthStencilResources();
	
	CreateFrameBuffers();
}

VulkanFrameBuffer::~VulkanFrameBuffer()
{
	for (int i = 0; i < sc.swapchainImageCount; i++)
	{
		vkDestroyFramebuffer(vk.device, handles.frameBuffers[i], nullptr);
	}

	delete(handles.colorImage);
	delete(handles.depthStencilImage);
}

void VulkanFrameBuffer::CreateColorResources()
{
	VulkanImageCreateInfo imageInfo{};
	imageInfo.width = sc.swapChainExtent.width;
	imageInfo.height = sc.swapChainExtent.height;
	imageInfo.mipLevels = 1;
	imageInfo.samples = msaaSamples;
	imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageInfo.imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	imageInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VulkanImageViewCreateInfo imageViewInfo{};
	imageViewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageViewInfo.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewInfo.mipLevels = 1;

	handles.colorImage = new VulkanImage(vk, imageInfo, imageViewInfo);
}

void VulkanFrameBuffer::CreateDepthStencilResources()
{
	VulkanImageCreateInfo imageInfo{};
	imageInfo.width = sc.swapChainExtent.width;
	imageInfo.height = sc.swapChainExtent.height;
	imageInfo.mipLevels = 1;
	imageInfo.samples = msaaSamples;
	imageInfo.format = VK_FORMAT_D32_SFLOAT_S8_UINT; 
	imageInfo.imageUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; 
	imageInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VulkanImageViewCreateInfo imageViewInfo{};
	imageViewInfo.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
	imageViewInfo.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	imageViewInfo.mipLevels = 1;

	handles.depthStencilImage = new VulkanImage(vk, imageInfo, imageViewInfo);
}

void VulkanFrameBuffer::CreateFrameBuffers()
{
	handles.frameBuffers.resize(sc.swapchainImageCount);

	for (int i = 0; i < sc.swapchainImageCount; i++)
	{
		VkImageView attachments[] = { handles.colorImage->getHandles().imageView,  handles.depthStencilImage->getHandles().imageView, sc.swapchainImageViews[i] };

		VkFramebufferCreateInfo frameBufferInfo{};
		frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferInfo.renderPass = rp.renderPass;
		frameBufferInfo.attachmentCount = 3;
		frameBufferInfo.pAttachments = attachments;
		frameBufferInfo.width = sc.swapChainExtent.width;
		frameBufferInfo.height = sc.swapChainExtent.height;
		frameBufferInfo.layers = 1;

		VK_CHECK(vkCreateFramebuffer(vk.device, &frameBufferInfo, nullptr, &handles.frameBuffers[i]), "FAILED TO CREATE FRAME BUFFER");
	}
}
