#pragma once

class MeshManager;

struct MeshRange
{
	uint32_t firstVertex;
	uint32_t vertexCount;
	VkDeviceSize vertexOffset;

	uint32_t firstIndex;
	VkDeviceSize indexOffset;
	uint32_t indexCount;
};

struct Mesh
{
	MeshRange meshRange;
	uint32_t textureId;
};

struct ModelHandles
{
	std::vector<Mesh*> meshes;
};

class Model
{
public:
	Model(const std::string& modelFilePath, MeshManager* meshManager);
	~Model();

	const std::vector<Mesh*> getMeshes() const { return handles.meshes; }
private:
	ModelHandles handles;
	
};
