#pragma once

#include <string>
#include <vector>
#include "vulkan/vulkan.h" // For VkDeviceSize

// Forward declarations
class MeshManager;
class TextureManager;

// Xác định một khoảng (range) trong vertex buffer và index buffer chung
// tương ứng với một mesh cụ thể.
struct MeshRange
{
	uint32_t firstVertex;
	uint32_t vertexCount;

	uint32_t firstIndex;
	uint32_t indexCount;
};

// Đại diện cho một mesh con trong một model.
// Chứa thông tin về vị trí trong buffer và ID của texture sẽ được áp dụng.
struct Mesh
{
	MeshRange meshRange;
	uint32_t materialIndex;
};

// Struct chứa dữ liệu nội bộ của một Model.
struct ModelHandles
{
	std::vector<Mesh*> meshes;
};

class MaterialManager;

// Class Model đại diện cho một đối tượng 3D có thể render, bao gồm một hoặc nhiều mesh.
// Class này chịu trách nhiệm điều phối việc điều phối việc tải dữ liệu model từ file
// và tạo ra các đối tượng Mesh tương ứng.
class Model
{
public:
	// Constructor tải model từ file và tạo các mesh thông qua các manager.
	Model(const std::string& modelFilePath, MeshManager* meshManager, MaterialManager* materialManager);
	
	// Destructor, chịu trách nhiệm giải phóng các đối tượng Mesh mà nó sở hữu.
	~Model();

	// Lấy danh sách các mesh con của model.
	const std::vector<Mesh*> getMeshes() const { return m_Handles.meshes; }
	
private:
	ModelHandles m_Handles;
	
};