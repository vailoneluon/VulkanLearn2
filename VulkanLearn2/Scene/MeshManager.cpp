#include "pch.h"
#include "MeshManager.h"

#include "Core/VulkanCommandManager.h"
#include "Model.h"
#include "Utils/ModelLoader.h"
#include "Core/VulkanBuffer.h"

MeshManager::MeshManager(const VulkanHandles& vulkanHandles, VulkanCommandManager* commandManager):
	vk(vulkanHandles), cmd(commandManager)
{
	
}

MeshManager::~MeshManager()
{
	delete(handles.vertexBuffer);
	delete(handles.indexBuffer);
}

std::vector<Mesh*> MeshManager::createMeshFromMeshData(const MeshData* meshData, uint32_t meshCount)
{
	std::vector<Mesh*> outMeshes;
	for (int i = 0; i < meshCount; i++)
	{
		uint32_t vertexIndexOffset = handles.allVertices.size();
		uint32_t indexIndexOffer = handles.allIndices.size();
		handles.allVertices.insert(handles.allVertices.end(), meshData[i].vertices.begin(), meshData[i].vertices.end());
		handles.allIndices.insert(handles.allIndices.end(), meshData[i].indices.begin(), meshData[i].indices.end());

		MeshRange meshRange;
		
		meshRange.firstVertex = vertexIndexOffset;
		meshRange.vertexOffset = vertexIndexOffset * sizeof(meshData[i].vertices[0]);

		meshRange.firstIndex = indexIndexOffer;
		meshRange.indexOffset = indexIndexOffer * sizeof(meshData[i].indices[0]);

		meshRange.vertexCount = meshData[i].vertices.size();
		meshRange.indexCount = meshData[i].indices.size();

		Mesh* mesh = new Mesh();
		mesh->meshRange = meshRange;

		outMeshes.push_back(mesh);
	}

	return outMeshes;
}

void MeshManager::CreateBuffers()
{
	CreateVertexBuffer();
	CreateIndexBuffer();
}

void MeshManager::CreateVertexBuffer()
{
	VkDeviceSize bufferSize = handles.allVertices.size() * sizeof(handles.allVertices[0]);

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.queueFamilyIndexCount = 1;
	bufferInfo.pQueueFamilyIndices = &vk.queueFamilyIndices.GraphicQueueIndex;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	handles.vertexBuffer = new VulkanBuffer(vk, cmd, bufferInfo, true);

	handles.vertexBuffer->UploadData(handles.allVertices.data(), bufferSize, 0);
}

void MeshManager::CreateIndexBuffer()
{
	VkDeviceSize bufferSize = handles.allIndices.size() * sizeof(handles.allIndices[0]);

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.queueFamilyIndexCount = 1;
	bufferInfo.pQueueFamilyIndices = &vk.queueFamilyIndices.GraphicQueueIndex;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

	handles.indexBuffer = new VulkanBuffer(vk, cmd, bufferInfo, true);

	handles.indexBuffer->UploadData(handles.allIndices.data(), bufferSize, 0);
}

