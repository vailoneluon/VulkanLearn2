#pragma once
#include "Core/VulkanContext.h"
#include "Core/VulkanBuffer.h"

class VulkanCommandManager;
class VulkanBuffer;
struct Mesh;
struct MeshData;

struct MeshManagerHandles
{
	VulkanBuffer* vertexBuffer;
	VulkanBuffer* indexBuffer;
	
	std::vector<Vertex> allVertices;
	std::vector<uint16_t> allIndices;
};

class MeshManager
{
public:
	MeshManager(const VulkanHandles& vulkanHandles, VulkanCommandManager* commandManager);
	~MeshManager();

	const MeshManagerHandles& getHandles() const { return handles; };
	const VkBuffer& getVertexBuffer() const { return handles.vertexBuffer->getHandles().buffer; }
	const VkBuffer& getIndexBuffer() const { return handles.indexBuffer->getHandles().buffer; }
	
	std::vector<Mesh*> createMeshFromMeshData(const MeshData* meshData, uint32_t meshCount);
	void CreateBuffers();
private:
	const VulkanHandles& vk;
	VulkanCommandManager* cmd;
	MeshManagerHandles handles;
	
	void CreateVertexBuffer();
	void CreateIndexBuffer();

};
