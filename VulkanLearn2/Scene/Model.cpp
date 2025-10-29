#include "pch.h"
#include "Model.h"
#include "MeshManager.h"
#include "Utils/ModelLoader.h"
#include "TextureManager.h"

Model::Model(const std::string& modelFilePath, MeshManager* meshManager, TextureManager* textureManager)
{
	ModelLoader modelLoader;
	ModelData modelData = modelLoader.LoadModelFromFile(modelFilePath);
	handles.meshes = meshManager->createMeshFromMeshData(modelData.meshData.data(), modelData.meshData.size());
	for (int i = 0; i < handles.meshes.size(); i++)
	{
		handles.meshes[i]->textureId = textureManager->LoadTextureImage("Resources/Textures/" + modelData.meshData[i].textureFilePath);
	}
}

Model::~Model()
{
	for (auto& mesh : handles.meshes)
	{
		delete(mesh);
	}
}
