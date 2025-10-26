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

class Application
{
public:
	Application();
	~Application();

	void Loop();
private:

	ModelLoader modelLoader;
	ModelData modelData{};

	void LoadModelFromFile(const string& filePath);

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

	VulkanBuffer* vertexBuffer;
	VulkanBuffer* indexBuffer;

	vector<VulkanDescriptor*> descriptors;
	VulkanImage* textureImage;
	VulkanDescriptor* textureImageDescriptor;

	UniformBufferObject ubo{};
	vector<VulkanBuffer*> uniformBuffers;
	vector<VulkanDescriptor*> uniformDescriptors;

	PushConstantData pushConstantData;

	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateTextureImage(const VulkanHandles& vk);
	void CreateUniformBuffer();
	void UpdateDescriptorBinding();

	void UpdateUniforms();

	void RecordCommandBuffer(VkCommandBuffer cmdBuffer, uint32_t imageIndex);
	void BindDescriptorSet(const VkCommandBuffer& cmdBuffer);
	void DrawFrame();

};