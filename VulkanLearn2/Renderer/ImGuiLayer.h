#pragma once
#include "Core/VulkanContext.h"
#include "vulkan/vulkan_core.h"

class VulkanSwapchain;
class Window;


class ImGuiLayer
{
public:
	ImGuiLayer(Window* window, const VulkanHandles& vulkanHandles, const VulkanSwapchain* vulkanSwapchain);
	~ImGuiLayer();

	void BeginFrame();

	void RenderFrame(VkCommandBuffer cmdBuffer, uint32_t imageIndex);

private:
	const VulkanHandles& m_VulkanHandles;
	const VulkanSwapchain* m_VulkanSwapchain;

	VkDescriptorPool m_ImGuiDescriptorPool;

	void Init(Window* window, const VulkanHandles& vulkanHandles, const VulkanSwapchain* vulkanSwapchain);

};
