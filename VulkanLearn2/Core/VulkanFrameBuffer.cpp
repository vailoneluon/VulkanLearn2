#include "pch.h"
#include "VulkanFrameBuffer.h"

// Cần include các file header này vì chúng ta sử dụng đầy đủ các struct của chúng.
#include "VulkanImage.h"
#include "VulkanRenderPass.h"

VulkanFrameBuffer::VulkanFrameBuffer(const VulkanHandles& vulkanHandles, const SwapchainHandles& swapchainHandles, const RenderPassHandles& renderPassHandles, const VkSampleCountFlagBits samples):
	m_VulkanHandles(vulkanHandles), 
	m_SwapchainHandles(swapchainHandles), 
	m_RenderPassHandles(renderPassHandles),
	m_MsaaSamples(samples)
{
	// Tuần tự tạo các tài nguyên cần thiết cho framebuffer.
	CreateColorResources();
	CreateDepthStencilResources();
	CreateFrameBuffers();
}

VulkanFrameBuffer::~VulkanFrameBuffer()
{
	// Hủy tất cả các framebuffer object.
	for (size_t i = 0; i < m_SwapchainHandles.swapchainImageCount; i++)
	{
		vkDestroyFramebuffer(m_VulkanHandles.device, m_Handles.frameBuffers[i], nullptr);
	}

	// Hủy các image attachment (color và depth) mà framebuffer này đã tạo và sở hữu.
	delete(m_Handles.colorImage);
	delete(m_Handles.depthStencilImage);
}

void VulkanFrameBuffer::CreateColorResources()
{
	// Tạo một image để dùng làm color attachment cho việc xử lý MSAA.
	// Kết quả render sẽ được vẽ vào image này, sau đó resolve vào image của swapchain.
	VulkanImageCreateInfo imageInfo{};
	imageInfo.width = m_SwapchainHandles.swapChainExtent.width;
	imageInfo.height = m_SwapchainHandles.swapChainExtent.height;
	imageInfo.mipLevels = 1;
	imageInfo.samples = m_MsaaSamples;
	imageInfo.format = m_SwapchainHandles.swapchainSupportDetails.chosenFormat.format;
	imageInfo.imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	imageInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VulkanImageViewCreateInfo imageViewInfo{};
	imageViewInfo.format = m_SwapchainHandles.swapchainSupportDetails.chosenFormat.format;
	imageViewInfo.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewInfo.mipLevels = 1;

	m_Handles.colorImage = new VulkanImage(m_VulkanHandles, imageInfo, imageViewInfo);
}

void VulkanFrameBuffer::CreateDepthStencilResources()
{
	// Tạo một image để dùng làm depth/stencil attachment.
	VulkanImageCreateInfo imageInfo{};
	imageInfo.width = m_SwapchainHandles.swapChainExtent.width;
	imageInfo.height = m_SwapchainHandles.swapChainExtent.height;
	imageInfo.mipLevels = 1;
	imageInfo.samples = m_MsaaSamples;
	imageInfo.format = VK_FORMAT_D32_SFLOAT_S8_UINT; // Format phổ biến cho depth/stencil.
	imageInfo.imageUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; 
	imageInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VulkanImageViewCreateInfo imageViewInfo{};
	imageViewInfo.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
	imageViewInfo.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	imageViewInfo.mipLevels = 1;

	m_Handles.depthStencilImage = new VulkanImage(m_VulkanHandles, imageInfo, imageViewInfo);
}

void VulkanFrameBuffer::CreateFrameBuffers()
{
	// Tạo một framebuffer cho mỗi image view trong swapchain.
	m_Handles.frameBuffers.resize(m_SwapchainHandles.swapchainImageCount);

	for (size_t i = 0; i < m_SwapchainHandles.swapchainImageCount; i++)
	{
		// Mảng các attachment view, thứ tự phải khớp với thứ tự trong subpass của render pass.
		// 0: Color (MSAA) -> 1: Depth/Stencil -> 2: Resolve (Swapchain Image)
		VkImageView attachments[] = { 
			m_Handles.colorImage->GetHandles().imageView,  
			m_Handles.depthStencilImage->GetHandles().imageView, 
			m_SwapchainHandles.swapchainImageViews[i] 
		};

		VkFramebufferCreateInfo frameBufferInfo{};
		frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferInfo.renderPass = m_RenderPassHandles.renderPass;
		frameBufferInfo.attachmentCount = 3;
		frameBufferInfo.pAttachments = attachments;
		frameBufferInfo.width = m_SwapchainHandles.swapChainExtent.width;
		frameBufferInfo.height = m_SwapchainHandles.swapChainExtent.height;
		frameBufferInfo.layers = 1;

		VK_CHECK(vkCreateFramebuffer(m_VulkanHandles.device, &frameBufferInfo, nullptr, &m_Handles.frameBuffers[i]), "LỖI: Tạo framebuffer thất bại!");
	}
}