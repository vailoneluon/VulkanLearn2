#pragma once

#include "Core/VulkanContext.h"
#include "Core/VulkanTypes.h"

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

	// Hộp
	/*
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
	*/

	// Kim tự tháp
	const std::vector<Vertex> vertices = {
		// 0: top (chóp)
		{{ 0.0f,  0.5f,  0.0f}, {1.0f, 1.0f, 1.0f}, {0.5f, 1.0f}}, // top

		// Front face (base vertices for front)
		{{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 1: front-left  (uv 0,0)
		{{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 2: front-right (uv 1,0)

		// Right face (duplicate front-right as left uv, and back-right as right uv)
		{{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, // 3: front-right (dup) (uv 0,0)
		{{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}, // 4: back-right  (uv 1,0)

		// Back face (duplicate back-right as left uv, and back-left as right uv)
		{{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, // 5: back-right (dup) (uv 0,0)
		{{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 6: back-left  (uv 1,0)

		// Left face (duplicate back-left as left uv, and front-left as right uv)
		{{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, // 7: back-left (dup) (uv 0,0)
		{{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, // 8: front-left (dup) (uv 1,0)

		// Bottom face (square) - give proper square UVs
		{{-0.5f, -0.5f,  0.5f}, {0.6f, 0.6f, 0.6f}, {0.0f, 0.0f}}, // 9: bottom front-left
		{{ 0.5f, -0.5f,  0.5f}, {0.6f, 0.6f, 0.6f}, {1.0f, 0.0f}}, // 10: bottom front-right
		{{ 0.5f, -0.5f, -0.5f}, {0.6f, 0.6f, 0.6f}, {1.0f, 1.0f}}, // 11: bottom back-right
		{{-0.5f, -0.5f, -0.5f}, {0.6f, 0.6f, 0.6f}, {0.0f, 1.0f}}, // 12: bottom back-left
	};
	const std::vector<uint16_t> indices = {
		// 4 side faces (each a triangle) - using top (0) + appropriate base verts
		0, 1, 2,   // front
		0, 3, 4,   // right
		0, 5, 6,   // back
		0, 7, 8,   // left

		// bottom (square) - two triangles
		9, 12, 11,
		11, 10, 9
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

	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateTextureImage(const VulkanHandles& vk);
	void CreateUniformBuffer();
	void UpdateUniforms();

	void RecordCommandBuffer(VkCommandBuffer cmdBuffer, uint32_t imageIndex);
	void DrawFrame();

};