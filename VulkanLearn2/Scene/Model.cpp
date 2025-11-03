#include "pch.h"
#include "Model.h"
#include "MeshManager.h"
#include "Utils/ModelLoader.h"
#include "TextureManager.h"

Model::Model(const std::string& modelFilePath, MeshManager* meshManager, TextureManager* textureManager)
{
	// 1. Dùng ModelLoader để tải dữ liệu thô (vertex, index, texture path) từ file model.
	ModelLoader modelLoader;
	ModelData modelData = modelLoader.LoadModelFromFile(modelFilePath);

	// 2. Dùng MeshManager để tạo các đối tượng Mesh từ dữ liệu thô.
	//    Hàm này sẽ đưa dữ liệu vertex/index vào các buffer chung và trả về các struct Mesh
	//    chứa thông tin về vị trí và kích thước của dữ liệu đó trong buffer.
	m_Handles.meshes = meshManager->createMeshFromMeshData(modelData.meshData.data(),static_cast<uint32_t>(modelData.meshData.size()));

	// 3. Với mỗi mesh, tải texture tương ứng thông qua TextureManager.
	for (int i = 0; i < m_Handles.meshes.size(); i++)
	{
		// Lấy ID của texture và gán vào mesh.
		// LƯU Ý: Đường dẫn texture đang được ghép nối cứng với prefix "Resources/Textures/".
		// Điều này có thể cần được cấu hình linh hoạt hơn trong tương lai.
		m_Handles.meshes[i]->textureId = textureManager->LoadTextureImage("Resources/Textures/" + modelData.meshData[i].textureFilePath);
	}
}

Model::~Model()
{
	// Class Model sở hữu các con trỏ Mesh* mà nó nhận từ MeshManager.
	// Do đó, destructor của Model có trách nhiệm giải phóng chúng.
	for (auto& mesh : m_Handles.meshes)
	{
		delete(mesh);
	}
}