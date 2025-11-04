#include "pch.h"
#include "VulkanFrameBuffer.h"

// Cần include các file header này vì chúng ta sử dụng đầy đủ các struct của chúng.
#include "VulkanImage.h"
#include "VulkanRenderPass.h"

VulkanFrameBuffer::VulkanFrameBuffer(const VulkanHandles& vulkanHandles, const SwapchainHandles& swapchainHandles, const RenderPassHandles& renderPassHandles, const VkSampleCountFlagBits samples, uint32_t maxFramesInFlight):
	m_VulkanHandles(vulkanHandles), 
	m_SwapchainHandles(swapchainHandles), 
	m_RenderPassHandles(renderPassHandles),
	m_MsaaSamples(samples),
	m_MaxFrameInFlight(maxFramesInFlight)
{
	// Tuần tự tạo các tài nguyên cần thiết cho framebuffer.
	CreateColorResources();
	CreateDepthStencilResources();
	CreateFrameBuffers();
	CreateRTTResources();
	CreateRTTFrameBuffers();
}

VulkanFrameBuffer::~VulkanFrameBuffer()
{
	// Hủy tất cả các framebuffer object.
	for (auto& frameBuffer : m_Handles.mainFrameBuffers)
	{
		vkDestroyFramebuffer(m_VulkanHandles.device, frameBuffer, nullptr);
	}

	for (auto& rttFrameBuffer : m_Handles.rttFrameBuffers)
	{
		vkDestroyFramebuffer(m_VulkanHandles.device, rttFrameBuffer, nullptr);
	}
	// Hủy các image attachment (color và depth) mà framebuffer này đã tạo và sở hữu.
	delete(m_Handles.colorImage);
	delete(m_Handles.depthStencilImage);
	
	for (auto& rttResolveImage : m_Handles.rttResolveImages)
	{
		delete(rttResolveImage);
	}
	delete(m_Handles.rttColorImage);
	delete(m_Handles.rttDepthStencilImage);

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
	m_Handles.mainFrameBuffers.resize(m_SwapchainHandles.swapchainImageCount);

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
		frameBufferInfo.renderPass = m_RenderPassHandles.mainRenderPass;
		frameBufferInfo.attachmentCount = 3;
		frameBufferInfo.pAttachments = attachments;
		frameBufferInfo.width = m_SwapchainHandles.swapChainExtent.width;
		frameBufferInfo.height = m_SwapchainHandles.swapChainExtent.height;
		frameBufferInfo.layers = 1;

		VK_CHECK(vkCreateFramebuffer(m_VulkanHandles.device, &frameBufferInfo, nullptr, &m_Handles.mainFrameBuffers[i]), "LỖI: Tạo framebuffer thất bại!");
	}
}

void VulkanFrameBuffer::CreateRTTResources()
{
	// Tạo RTT Resolve Image
	m_Handles.rttResolveImages.resize(m_MaxFrameInFlight);
	VulkanImageCreateInfo ResolveImageInfo{};
	ResolveImageInfo.width = m_SwapchainHandles.swapChainExtent.width;
	ResolveImageInfo.height = m_SwapchainHandles.swapChainExtent.height;
	ResolveImageInfo.mipLevels = 1;
	ResolveImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	ResolveImageInfo.format = m_SwapchainHandles.swapchainSupportDetails.chosenFormat.format;
	ResolveImageInfo.imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	ResolveImageInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VulkanImageViewCreateInfo ResolveImageViewInfo{};
	ResolveImageViewInfo.format = m_SwapchainHandles.swapchainSupportDetails.chosenFormat.format;
	ResolveImageViewInfo.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	ResolveImageViewInfo.mipLevels = 1;

	for (int i = 0; i < m_MaxFrameInFlight; i++)
	{
		m_Handles.rttResolveImages[i] = new VulkanImage(m_VulkanHandles, ResolveImageInfo, ResolveImageViewInfo);
	}

	// Tao Color Image
	VulkanImageCreateInfo ColorImageInfo{};
	ColorImageInfo.width = m_SwapchainHandles.swapChainExtent.width;
	ColorImageInfo.height = m_SwapchainHandles.swapChainExtent.height;
	ColorImageInfo.mipLevels = 1;
	ColorImageInfo.samples = m_MsaaSamples;
	ColorImageInfo.format = m_SwapchainHandles.swapchainSupportDetails.chosenFormat.format;
	ColorImageInfo.imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	ColorImageInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VulkanImageViewCreateInfo ColorImageViewInfo{};
	ColorImageViewInfo.format = m_SwapchainHandles.swapchainSupportDetails.chosenFormat.format;
	ColorImageViewInfo.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	ColorImageViewInfo.mipLevels = 1;

	m_Handles.rttColorImage = new VulkanImage(m_VulkanHandles, ColorImageInfo, ColorImageViewInfo);

	// Tạo RTT Depth Image
	VulkanImageCreateInfo DepthImageInfo{};
	DepthImageInfo.width = m_SwapchainHandles.swapChainExtent.width;
	DepthImageInfo.height = m_SwapchainHandles.swapChainExtent.height;
	DepthImageInfo.mipLevels = 1;
	DepthImageInfo.samples = m_MsaaSamples;
	DepthImageInfo.format = VK_FORMAT_D32_SFLOAT_S8_UINT; // Format phổ biến cho depth/stencil.
	DepthImageInfo.imageUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	DepthImageInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VulkanImageViewCreateInfo DepthImageViewInfo{};
	DepthImageViewInfo.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
	DepthImageViewInfo.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	DepthImageViewInfo.mipLevels = 1;

	m_Handles.rttDepthStencilImage = new VulkanImage(m_VulkanHandles, DepthImageInfo, DepthImageViewInfo);
}

void VulkanFrameBuffer::CreateRTTFrameBuffers()
{
	// Render Pass RTT
	m_Handles.rttFrameBuffers.resize(m_MaxFrameInFlight);

	for (size_t i = 0; i < m_MaxFrameInFlight; i++)
	{
		// Mảng các attachment view, thứ tự phải khớp với thứ tự trong subpass của render pass.
		// 0:RTT Color (MSAA) -> 1:RTT Depth/Stencil -> 2: Resolve Image
		VkImageView attachments[] = {
			m_Handles.rttColorImage->GetHandles().imageView,
			m_Handles.rttDepthStencilImage->GetHandles().imageView,
			m_Handles.rttResolveImages[i]->GetHandles().imageView
		};

		VkFramebufferCreateInfo frameBufferInfo{};
		frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferInfo.renderPass = m_RenderPassHandles.rttRenderPass;
		frameBufferInfo.attachmentCount = 3;
		frameBufferInfo.pAttachments = attachments;
		frameBufferInfo.width = m_SwapchainHandles.swapChainExtent.width;
		frameBufferInfo.height = m_SwapchainHandles.swapChainExtent.height;
		frameBufferInfo.layers = 1;

		VK_CHECK(vkCreateFramebuffer(m_VulkanHandles.device, &frameBufferInfo, nullptr, &m_Handles.rttFrameBuffers[i]), "LỖI: Tạo framebuffer thất bại!");
	}
}
