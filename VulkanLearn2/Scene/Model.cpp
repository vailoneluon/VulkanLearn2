#include "pch.h"
#include "Model.h"
#include "MeshManager.h"
#include "Utils/ModelLoader.h"
#include "MaterialManager.h"

Model::Model(const std::string& modelFilePath, MeshManager* meshManager, MaterialManager* materialManager)
{
	// ModelLoader giờ đây sẽ nhận các manager và trực tiếp xử lý việc tạo Mesh và Material.
	ModelLoader modelLoader(meshManager, materialManager);
	m_Handles.meshes = modelLoader.LoadModelFromFile(modelFilePath);
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