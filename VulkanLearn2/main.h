#pragma once
#include "Core/Window.h"
#include "Core/VulkanContext.h"
#include "Core/VulkanSwapchain.h"
#include "Core/VulkanImage.h"
#include "Core/VulkanRenderPass.h"
#include "Core/VulkanFrameBuffer.h"
#include "Core/VulkanCommandManager.h"
#include "Core/VulkanPipeline.h"
#include "Core/VulkanSyncManager.h"
#include "Core/VulkanBuffer.h"
#include "Core/VulkanTypes.h"
#include "Core/VulkanDescriptorManager.h"

class Application
{
public:
	Application();
	~Application();

	void Loop();
private:

	const vector<Vertex> vertices = {
		// Mặt trước (Z = +0.5)
		{{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, // bottom-left
		{{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // bottom-right
		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // top-right
		{{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}, // top-left

		// Mặt sau (Z = -0.5)
		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, // bottom-right
		{{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}, // top-right
		{{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // top-left
		{{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, // bottom-left

		// Mặt trái (X = -0.5)
		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
		{{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
		{{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},

		// Mặt phải (X = +0.5)
		{{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
		{{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

		// Mặt trên (Y = +0.5)
		{{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
		{{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},

		// Mặt dưới (Y = -0.5)
		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	};

	const std::vector<uint16_t> indices = {
		// Mặt trước
		0, 1, 2, 2, 3, 0,
		// Mặt sau
		4, 5, 6, 6, 7, 4,
		// Mặt trái
		8, 9, 10, 10, 11, 8,
		// Mặt phải
		12, 13, 14, 14, 15, 12,
		// Mặt trên
		16, 17, 18, 18, 19, 16,
		// Mặt dưới
		20, 21, 22, 22, 23, 20
	};

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
	VulkanDescriptorManager* vulkanDescriptorManager;
	VulkanPipeline* vulkanPipeline;
	VulkanSyncManager* vulkanSyncManager;

	VulkanHandles vulkanHandles;
	SwapchainHandles swapchainHandles;
	RenderPassHandles renderPassHandles;
	FrameBufferHandles frameBufferHandles;
	CommandManagerHandles commandManagerHandles;
	DescriptorManagerHandles descriptorManagerHandles;
	PipelineHandles pipelineHandles;


	VulkanBuffer* vertexBuffer;
	VulkanBuffer* indexBuffer;

	UniformBufferObject ubo{};

	VulkanImage* textureImage;

	void CreateCacheHandles();

	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateTextureImage();
	void UpdateUniforms();

	void RecordCommandBuffer(VkCommandBuffer cmdBuffer, uint32_t imageIndex);
	void DrawFrame();

};
