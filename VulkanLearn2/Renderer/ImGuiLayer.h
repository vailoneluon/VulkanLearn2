#pragma once
#include "Core/VulkanContext.h"
#include "vulkan/vulkan_core.h"

class VulkanSwapchain;
class Window;
class VulkanImage;
class VulkanSampler;

class ImGuiLayer
{
public:
	ImGuiLayer(Window* window, const VulkanHandles& vulkanHandles, const VulkanSwapchain* vulkanSwapchain, 
		const std::vector<VulkanImage*>* sceneImage, const VulkanSampler* vulkanSampler);
	~ImGuiLayer();

	void BeginFrame();

	void RenderFrame(VkCommandBuffer cmdBuffer, uint32_t imageIndex, uint32_t currentFrame);
	void UpdateViewports();

private:
	const VulkanHandles& m_VulkanHandles;
	const VulkanSwapchain* m_VulkanSwapchain;

	//const std::vector<VulkanImage*>* m_SceneImage;
	std::vector<VkDescriptorSet> m_SceneImageDescriptorSet;

	VkDescriptorPool m_ImGuiDescriptorPool;

	void Init(Window* window, const VulkanHandles& vulkanHandles, const VulkanSwapchain* vulkanSwapchain);

	void RegisterSceneImage(const std::vector<VulkanImage*>* sceneImage, const VulkanSampler* vulkanSampler);

	void RecordDrawCommandBuffer(VkCommandBuffer cmdBuffer, uint32_t imageIndex);
};
