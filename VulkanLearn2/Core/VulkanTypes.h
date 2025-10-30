#pragma once

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;

	static std::array<VkVertexInputBindingDescription, 1> GetBindingDesc()
	{
		std::array<VkVertexInputBindingDescription, 1> bindingDesc{};
		bindingDesc[0].binding = 0;
		bindingDesc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		bindingDesc[0].stride = sizeof(Vertex);

		return bindingDesc;
	}

	static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDesc()
	{
		std::array<VkVertexInputAttributeDescription, 3> attributeDescs{};

		attributeDescs[0].binding = 0;
		attributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescs[0].location = 0;
		attributeDescs[0].offset = offsetof(Vertex, pos);

		attributeDescs[1].binding = 0;
		attributeDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescs[1].location = 1;
		attributeDescs[1].offset = offsetof(Vertex, normal);

		attributeDescs[2].binding = 0;
		attributeDescs[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescs[2].location = 2;
		attributeDescs[2].offset = offsetof(Vertex, uv);

		return attributeDescs;
	}
};

struct UniformBufferObject
{
	//alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct PushConstantData
{
	alignas(16) glm::mat4 model;
	alignas(16) uint32_t textureId;
};

struct DynamicBufferObject
{
	alignas(16) glm::mat4 model;

};