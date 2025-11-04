#pragma once
#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanRenderPass.h"

// Forward declarations
class VulkanImage;

// Struct chứa các handle và tài nguyên liên quan đến Framebuffer.
struct FrameBufferHandles 
{
	// Mỗi image trong swapchain sẽ có một framebuffer tương ứng.
	std::vector<VkFramebuffer> mainFrameBuffers;

	// Các attachment không thuộc swapchain (được tạo và quản lý bởi Framebuffer).
	VulkanImage* colorImage = nullptr; // Dành cho MSAA.
	VulkanImage* depthStencilImage = nullptr;

	// Attachment cho RTT subpass
	std::vector<VkFramebuffer> rttFrameBuffers;
	std::vector<VulkanImage*> rttResolveImages;
	VulkanImage* rttColorImage;
	VulkanImage* rttDepthStencilImage;
};

// Class quản lý việc tạo và hủy các VkFramebuffer.
// Một framebuffer là một tập hợp các attachment (color, depth, stencil, v.v.)
// mà một render pass sẽ vẽ vào.
class VulkanFrameBuffer
{
public:
	VulkanFrameBuffer(const VulkanHandles& vulkanHandles, const SwapchainHandles& swapchainHandles, const RenderPassHandles& renderPassHandles, const VkSampleCountFlagBits samples, uint32_t maxFramesInFlight);
	~VulkanFrameBuffer();

	// Lấy các handle nội bộ.
	const FrameBufferHandles& getHandles() const { return m_Handles; }

private:
	// --- Dữ liệu nội bộ ---
	FrameBufferHandles m_Handles;
	uint32_t m_MaxFrameInFlight;

	// --- Tham chiếu Vulkan ---
	const VulkanHandles& m_VulkanHandles;
	const SwapchainHandles& m_SwapchainHandles;
	const RenderPassHandles& m_RenderPassHandles;
	const VkSampleCountFlagBits m_MsaaSamples;
	
	// --- Hàm helper private ---
	void CreateColorResources();
	void CreateDepthStencilResources();
	void CreateFrameBuffers();

	void CreateRTTResources();
	void CreateRTTFrameBuffers();
};