#pragma once
#include "VulkanContext.h"

// Forward declaration
struct SwapchainHandles;

// Struct chứa handle nội bộ của VulkanRenderPass.
struct RenderPassHandles
{
	VkRenderPass mainRenderPass = VK_NULL_HANDLE;
	VkRenderPass rttRenderPass = VK_NULL_HANDLE;
	VkRenderPass brightRenderPass = VK_NULL_HANDLE;
};

// Class quản lý việc tạo và hủy một VkRenderPass.
// Render Pass mô tả cấu trúc của một quá trình render, bao gồm các attachment
// (ví dụ: color, depth), các subpass, và dependency giữa chúng.
class VulkanRenderPass
{
public:
	VulkanRenderPass(const VulkanHandles& vulkanHandles, const SwapchainHandles& swapchainHandles, VkSampleCountFlagBits msaaSamples);
	~VulkanRenderPass();

	// Lấy handle nội bộ.
	const RenderPassHandles& getHandles() const { return m_Handles; }

private:
	// --- Dữ liệu nội bộ ---
	RenderPassHandles m_Handles;

	// --- Tham chiếu Vulkan ---
	const VulkanHandles& m_VulkanHandles;
	const SwapchainHandles& m_SwapchainHandles;
	VkSampleCountFlagBits m_msaaSamples;


	void CreateRttRenderPass();
	void CreateMainRenderPass();
	void CreateBrightRenderPass();
};