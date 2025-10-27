#include "pch.h"
#include "Model.h"
#include "MeshManager.h"
#include "Utils/ModelLoader.h"

Model::Model(const std::string& modelFilePath, MeshManager* meshManager)
{
	ModelLoader modelLoader;
	ModelData modelData = modelLoader.LoadModelFromFile(modelFilePath);
	handles.meshes = meshManager->createMeshFromMeshData(modelData.meshData.data(), modelData.meshData.size());
}

Model::~Model()
{
	for (auto& mesh : handles.meshes)
	{
		delete(mesh);
	}
}
