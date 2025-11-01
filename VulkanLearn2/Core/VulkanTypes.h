#pragma once

struct InstanceData
{
	glm::mat4 modelOffset;
};

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;

	static std::array<VkVertexInputBindingDescription, 2> GetBindingDesc()
	{
		std::array<VkVertexInputBindingDescription, 2> bindingDesc{};
		bindingDesc[0].binding = 0;
		bindingDesc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		bindingDesc[0].stride = sizeof(Vertex);

		bindingDesc[1].binding = 1;
		bindingDesc[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
		bindingDesc[1].stride = sizeof(InstanceData);

		return bindingDesc;
	}

	static std::array<VkVertexInputAttributeDescription, 7> GetAttributeDesc()
	{
		std::array<VkVertexInputAttributeDescription, 7> attributeDescs{};

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

		// Description cho glm::mat4 model
		for (int i = 0; i < 4; i++)
		{
			attributeDescs[3 + i].binding = 1;
			attributeDescs[3 + i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescs[3 + i].location = i + 3;
			attributeDescs[3 + i].offset = sizeof(glm::vec4) * i;
		}

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

