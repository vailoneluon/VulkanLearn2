#include "pch.h"
#include "ModelLoader.h"
#include <stdexcept>
#include <string>

// --- Khai báo các hàm helper ---
namespace {
	void ProcessNode(aiNode* node, const aiScene* scene, ModelData& modelData);
	MeshData ProcessMesh(aiMesh* mesh, const aiScene* scene);
	std::string GetFileNameFromPath(const std::string& fullPath);
}

// --- Triển khai ModelLoader ---

ModelLoader::ModelLoader() {}
ModelLoader::~ModelLoader() {}

ModelData ModelLoader::LoadModelFromFile(const std::string& filePath)
{
	ModelData modelData;

	Assimp::Importer importer;

	unsigned int flags = 
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_GenSmoothNormals |
		//aiProcess_FlipUVs |
		aiProcess_CalcTangentSpace;

	// Kiểm tra xem file có phải là .assbin không
	if (filePath.rfind(".assbin") == (filePath.size() - 7))
	{
		flags = 0;
	}

	// Sử dụng 'flags' đã được cập nhật
	const aiScene* scene = importer.ReadFile(filePath, flags);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		throw std::runtime_error("ASSIMP ERROR: " + std::string(importer.GetErrorString()));
	}

	ProcessNode(scene->mRootNode, scene, modelData);

	return modelData;
}

// --- Định nghĩa các hàm helper ---
namespace {

	void ProcessNode(aiNode* node, const aiScene* scene, ModelData& modelData)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			modelData.meshData.push_back(ProcessMesh(mesh, scene));
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], scene, modelData);
		}
	}

	MeshData ProcessMesh(aiMesh* mesh, const aiScene* scene)
	{
		MeshData currentMeshData;

		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;

			vertex.pos.x = mesh->mVertices[i].x;
			vertex.pos.y = mesh->mVertices[i].y;
			vertex.pos.z = mesh->mVertices[i].z;

			if (mesh->HasNormals())
			{
				vertex.normal.x = mesh->mNormals[i].x;
				vertex.normal.y = mesh->mNormals[i].y;
				vertex.normal.z = mesh->mNormals[i].z;
			}
			else
			{
				vertex.normal = { 0.0f, 0.0f, 0.0f };
			}

			if (mesh->mTextureCoords[0])
			{
				vertex.uv.x = mesh->mTextureCoords[0][i].x;
				vertex.uv.y = mesh->mTextureCoords[0][i].y;
			}
			else
			{
				vertex.uv = { 0.0f, 0.0f };
			}

			currentMeshData.vertices.push_back(vertex);
		}

		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
			{
				currentMeshData.indices.push_back(face.mIndices[j]);
			}
		}

		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		if (material)
		{
			aiString texPath;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
			{
				currentMeshData.textureFilePath = GetFileNameFromPath(texPath.C_Str());
			}
		}

		return currentMeshData;
	}

	std::string GetFileNameFromPath(const std::string& fullPath)
	{
		size_t lastSlash = fullPath.find_last_of("/\\");

		if (lastSlash == std::string::npos)
		{
			return fullPath;
		}
		else
		{
			return fullPath.substr(lastSlash + 1);
		}
	}
}