#pragma once

#include "Core/VulkanContext.h"
#include "Core/VulkanBuffer.h"


// Forward declarations
class VulkanCommandManager;
struct Mesh;
struct MeshData;

// Struct chứa các buffer và dữ liệu CPU cho tất cả các mesh được quản lý.
struct MeshManagerHandles
{
	VulkanBuffer* vertexBuffer = nullptr;
	VulkanBuffer* indexBuffer = nullptr;
	
	// Dữ liệu vertex và index của tất cả các mesh được gộp lại.
	std::vector<Vertex> allVertices;
	std::vector<uint32_t> allIndices;
};

// Class quản lý việc tổng hợp dữ liệu mesh từ nhiều model khác nhau
// vào một Vertex Buffer và một Index Buffer duy nhất.
// Việc này giúp tối ưu hóa hiệu năng render bằng cách giảm số lượng lệnh bind buffer.
class MeshManager
{
public:
	MeshManager(const VulkanHandles& vulkanHandles, VulkanCommandManager* commandManager);
	~MeshManager();

	// --- Getters ---
	const MeshManagerHandles& getHandles() const { return m_Handles; };
	const VkBuffer& getVertexBuffer() const { return m_Handles.vertexBuffer->getHandles().buffer; }
	const VkBuffer& getIndexBuffer() const { return m_Handles.indexBuffer->getHandles().buffer; }
	
	// Gộp dữ liệu từ một mảng MeshData vào các vector tổng.
	// Trả về một vector các đối tượng Mesh chứa thông tin offset và count.
	std::vector<Mesh*> createMeshFromMeshData(const MeshData* meshData, uint32_t meshCount);

	// Tạo các buffer trên GPU từ dữ liệu đã được tổng hợp.
	void CreateBuffers();

private:
	// --- Tham chiếu Vulkan ---
	const VulkanHandles& m_VulkanHandles;
	VulkanCommandManager* m_CommandManager;

	// --- Dữ liệu nội bộ ---
	MeshManagerHandles m_Handles;
	
	// --- Hàm helper private ---
	void CreateVertexBuffer();
	void CreateIndexBuffer();

};