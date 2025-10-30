#include "pch.h"
#include "MeshManager.h"

#include "Core/VulkanCommandManager.h"
#include "Model.h"
#include "Utils/ModelLoader.h"
#include "Core/VulkanBuffer.h"

MeshManager::MeshManager(const VulkanHandles& vulkanHandles, VulkanCommandManager* commandManager):
	m_VulkanHandles(vulkanHandles), 
	m_CommandManager(commandManager)
{
}

MeshManager::~MeshManager()
{
	// Giải phóng các buffer đã được tạo.
	delete(m_Handles.vertexBuffer);
	delete(m_Handles.indexBuffer);
}

std::vector<Mesh*> MeshManager::createMeshFromMeshData(const MeshData* meshData, uint32_t meshCount)
{
	std::vector<Mesh*> outMeshes;
	outMeshes.reserve(meshCount);

	// --- Tối ưu hóa vòng lặp ---
	// Tính toán trước tổng số vertex và index sẽ được thêm vào.
	size_t totalVerticesToReserve = 0;
	size_t totalIndicesToReserve = 0;
	for (uint32_t i = 0; i < meshCount; i++)
	{
		totalVerticesToReserve += meshData[i].vertices.size();
		totalIndicesToReserve += meshData[i].indices.size();
	}

	
	m_Handles.allVertices.reserve(m_Handles.allVertices.size() + totalVerticesToReserve);
	m_Handles.allIndices.reserve(m_Handles.allIndices.size() + totalIndicesToReserve);
	

	for (uint32_t i = 0; i < meshCount; i++)
	{
		// Ghi lại offset hiện tại trước khi thêm dữ liệu mới.
		uint32_t vertexIndexOffset = m_Handles.allVertices.size();
		uint32_t indexIndexOffset = m_Handles.allIndices.size();

		// Nối dữ liệu vertex và index của mesh hiện tại vào vector tổng.
		m_Handles.allVertices.insert(m_Handles.allVertices.end(), meshData[i].vertices.begin(), meshData[i].vertices.end());
		m_Handles.allIndices.insert(m_Handles.allIndices.end(), meshData[i].indices.begin(), meshData[i].indices.end());

		// Tạo một đối tượng MeshRange để lưu thông tin về vị trí và kích thước
		// của dữ liệu vừa thêm vào trong các buffer tổng.
		MeshRange meshRange;
		
		meshRange.firstVertex = vertexIndexOffset;
		meshRange.vertexOffset = vertexIndexOffset * sizeof(sizeof(meshData[i].vertices[0])); 

		meshRange.firstIndex = indexIndexOffset;
		meshRange.indexOffset = indexIndexOffset * sizeof(sizeof(meshData[i].indices[0])); 

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
	// Chỉ tạo buffer nếu có dữ liệu.
	if (!m_Handles.allVertices.empty())
	{
		CreateVertexBuffer();
	}
	if (!m_Handles.allIndices.empty())
	{
		CreateIndexBuffer();
	}
}

void MeshManager::CreateVertexBuffer()
{
	VkDeviceSize bufferSize = m_Handles.allVertices.size() * sizeof(m_Handles.allVertices[0]);

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.queueFamilyIndexCount = 1;
	bufferInfo.pQueueFamilyIndices = &m_VulkanHandles.queueFamilyIndices.GraphicQueueIndex;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	// LƯU Ý: Tham số `true` tạo ra một buffer có thể được map và ghi trực tiếp từ CPU (host-visible).
	// Điều này tiện lợi nhưng không phải là tối ưu nhất về hiệu năng.
	// Để tối ưu, nên tạo buffer này với cờ VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	// và sử dụng một staging buffer trung gian để copy dữ liệu từ CPU sang GPU.
	m_Handles.vertexBuffer = new VulkanBuffer(m_VulkanHandles, m_CommandManager, bufferInfo, true);

	m_Handles.vertexBuffer->UploadData(m_Handles.allVertices.data(), bufferSize, 0);
}

void MeshManager::CreateIndexBuffer()
{
	VkDeviceSize bufferSize = m_Handles.allIndices.size() * sizeof(m_Handles.allIndices[0]);

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.queueFamilyIndexCount = 1;
	bufferInfo.pQueueFamilyIndices = &m_VulkanHandles.queueFamilyIndices.GraphicQueueIndex;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

	// LƯU Ý: Tương tự như Vertex Buffer, Index Buffer cũng nên được đặt trong device-local memory
	// và được ghi dữ liệu thông qua một staging buffer để đạt hiệu năng cao nhất.
	m_Handles.indexBuffer = new VulkanBuffer(m_VulkanHandles, m_CommandManager, bufferInfo, true);

	m_Handles.indexBuffer->UploadData(m_Handles.allIndices.data(), bufferSize, 0);
}