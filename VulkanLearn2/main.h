#pragma once

#include "Core/VulkanContext.h"
#include "Core/VulkanTypes.h"
#include "Utils/ModelLoader.h"

// Forward declare
class Window;
class VulkanSwapchain;
class VulkanRenderPass;
class VulkanFrameBuffer;
class VulkanCommandManager;
class VulkanSampler;
class VulkanDescriptorManager;
class VulkanPipeline;
class VulkanSyncManager;
class VulkanBuffer;
class VulkanImage;
class VulkanDescriptor;
class RenderObject;
class MeshManager;
class TextureManager;

class Application
{
public:
	Application();
	~Application();

	void Loop();
private:

	RenderObject* obj1;
	RenderObject* obj2;
	std::vector<RenderObject*> renderObjects;


	//////////////////////////////////////////////////////////////////////////////////////////////

	const uint32_t WINDOW_WIDTH = 800;
	const uint32_t WINDOW_HEIGHT = 600;

	const VkClearColorValue backgroundColor = { 0.1f, 0.1f, 0.2f, 1.0f };

	const VkSampleCountFlagBits MSAA_SAMPLES = VK_SAMPLE_COUNT_4_BIT;
	const int MAX_FRAMES_IN_FLIGHT = 2;

	int currentFrame = 0;

	Window* window;
	VulkanContext* vulkanContext;
	VulkanSwapchain* vulkanSwapchain;
	VulkanRenderPass* vulkanRenderPass;
	VulkanFrameBuffer* vulkanFrameBuffer;

	VulkanCommandManager* vulkanCommandManager;
	VulkanSampler* vulkanSampler;
	VulkanDescriptorManager* vulkanDescriptorManager;
	VulkanPipeline* vulkanPipeline;
	VulkanSyncManager* vulkanSyncManager;

	MeshManager* meshManager;
	TextureManager* textureManager;

	std::vector<VulkanDescriptor*> descriptors;

	UniformBufferObject ubo{};
	std::vector<VulkanBuffer*> uniformBuffers;
	std::vector<VulkanDescriptor*> uniformDescriptors;

	PushConstantData pushConstantData;

	void CreateUniformBuffer();
	void UpdateDescriptorBinding();

	void UpdateUniforms();
	void UpdateRenderObjectTransform();

	void RecordCommandBuffer(const VkCommandBuffer& cmdBuffer, uint32_t imageIndex);
	void CmdDrawRenderObjects(const VkCommandBuffer& cmdBuffer);
	void BindDescriptorSet(const VkCommandBuffer& cmdBuffer);
	void DrawFrame();

};