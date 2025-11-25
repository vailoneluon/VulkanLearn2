#pragma once

// =================================================================================================
// Struct: Vertex
// Mô tả: Định nghĩa cấu trúc của một đỉnh (vertex) trong không gian 3D.
//        Bao gồm vị trí, pháp tuyến, tọa độ UV và tiếp tuyến.
// =================================================================================================
struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
	glm::vec3 tangent;

	// Helper: Lấy mô tả về binding của vertex buffer.
	static std::array<VkVertexInputBindingDescription, 1> GetBindingDesc()
	{
		std::array<VkVertexInputBindingDescription, 1> bindingDesc{};
		bindingDesc[0].binding = 0;
		bindingDesc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		bindingDesc[0].stride = sizeof(Vertex);

		return bindingDesc;
	}

	// Helper: Lấy mô tả về các thuộc tính (attribute) của vertex.
	static std::array<VkVertexInputAttributeDescription, 4> GetAttributeDesc()
	{
		std::array<VkVertexInputAttributeDescription, 4> attributeDescs{};

		// Attribute 0: Position
		attributeDescs[0].binding = 0;
		attributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescs[0].location = 0; 
		attributeDescs[0].offset = offsetof(Vertex, pos);

		// Attribute 1: Normal
		attributeDescs[1].binding = 0;
		attributeDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescs[1].location = 1;
		attributeDescs[1].offset = offsetof(Vertex, normal);

		// Attribute 2: UV
		attributeDescs[2].binding = 0;
		attributeDescs[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescs[2].location = 2;
		attributeDescs[2].offset = offsetof(Vertex, uv);

		// Attribute 3: Tangent
		attributeDescs[3].binding = 0;
		attributeDescs[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescs[3].location = 3;
		attributeDescs[3].offset = offsetof(Vertex, tangent);

		return attributeDescs;
	}
};

// =================================================================================================
// Struct: UniformBufferObject
// Mô tả: Chứa các ma trận biến đổi cơ bản cho camera.
//        Cần align 16 byte để tương thích với std140 layout của Vulkan.
// =================================================================================================
struct UniformBufferObject
{
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::vec3 viewPos;
};

// =================================================================================================
// Struct: PushConstantData
// Mô tả: Dữ liệu push constant gửi cho shader mỗi lần draw call.
//        Thường chứa ma trận model và index vật liệu.
// =================================================================================================
struct PushConstantData
{
	alignas(16) glm::mat4 model;
	alignas(16) uint32_t materialIndex;
};


// =================================================================================================
// Struct: ShadowMapPushConstantData
// Mô tả: Dữ liệu push constant dùng riêng cho pass tạo shadow map.
// =================================================================================================
struct ShadowMapPushConstantData
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 lightMatrix;
};