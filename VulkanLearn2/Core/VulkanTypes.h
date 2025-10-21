#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <vulkan/vulkan.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <array>

using namespace std;

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 uv;

	static VkVertexInputBindingDescription GetBindingDesc()
	{
		VkVertexInputBindingDescription bindingDesc{};
		bindingDesc.binding = 0;
		bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		bindingDesc.stride = sizeof(Vertex);

		return bindingDesc;
	}

	static array<VkVertexInputAttributeDescription, 3> GetAttributeDesc()
	{
		array<VkVertexInputAttributeDescription, 3> attributeDescs{};

		attributeDescs[0].binding = 0;
		attributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescs[0].location = 0;
		attributeDescs[0].offset = offsetof(Vertex, pos);

		attributeDescs[1].binding = 0;
		attributeDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescs[1].location = 1;
		attributeDescs[1].offset = offsetof(Vertex, color);

		attributeDescs[2].binding = 0;
		attributeDescs[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescs[2].location = 2;
		attributeDescs[2].offset = offsetof(Vertex, uv);

		return attributeDescs;
	}
};

struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};