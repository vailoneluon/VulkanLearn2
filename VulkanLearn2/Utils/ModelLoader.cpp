#include "pch.h"
#include "ModelLoader.h"
#include "Core/VulkanContext.h" // Include để có định nghĩa đầy đủ của struct Vertex
#include "Scene/MeshManager.h"
#include "Scene/Model.h"
#include <stdexcept>

ModelLoader::ModelLoader(MeshManager* meshManager, MaterialManager* materialManager)
	: m_MeshManager(meshManager), m_MaterialManager(materialManager)
{
}

std::vector<Mesh*> ModelLoader::LoadModelFromFile(const std::string& filePath)
{
	std::vector<Mesh*> meshes;
	Assimp::Importer importer;

	// Các cờ xử lý hậu kỳ của Assimp.
	unsigned int flags =
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_GenSmoothNormals |
		aiProcess_CalcTangentSpace;

	if (filePath.size() > 7 && filePath.substr(filePath.size() - 7) == ".assbin")
	{
		flags = 0;
	}

	const aiScene* scene = importer.ReadFile(filePath, flags);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		throw std::runtime_error("LỖI ASSIMP: " + std::string(importer.GetErrorString()));
	}

	ProcessNode(scene->mRootNode, scene, meshes);

	return meshes;
}

void ModelLoader::ProcessNode(aiNode* node, const aiScene* scene, std::vector<Mesh*>& outMeshes)
{
	// Xử lý tất cả các mesh trong node hiện tại.
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		outMeshes.push_back(ProcessMesh(mesh, scene));
	}

	// Duyệt đệ quy qua tất cả các node con.
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(node->mChildren[i], scene, outMeshes);
	}
}

Mesh* ModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
	MeshData currentMeshData;

	// Trích xuất dữ liệu đỉnh (vị trí, pháp tuyến, UV).
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;
		vertex.pos = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
		if (mesh->HasNormals())
		{
			vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
		}
		if (mesh->mTextureCoords[0])
		{
			vertex.uv = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		}
		if (mesh->HasTangentsAndBitangents())
		{
			vertex.tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
		}
		currentMeshData.vertices.push_back(vertex);
	}

	// Trích xuất dữ liệu chỉ số (index).
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			currentMeshData.indices.push_back(face.mIndices[j]);
		}
	}

	// 1. Dùng MeshManager để tạo đối tượng Mesh từ MeshData.
	//    Hàm này sẽ gộp vertex/index data vào buffer chung và trả về một Mesh*
	//    với thông tin MeshRange đã được điền.
	std::vector<Mesh*> createdMeshes = m_MeshManager->createMeshFromMeshData(&currentMeshData, 1);
	Mesh* newMesh = createdMeshes[0];

	// 2. Trích xuất thông tin material từ Assimp.
	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	MaterialRawData materialRawData;
	if (material)
	{
		aiString texPath;
		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
		{
			std::string fileName = GetFileNameFromPath(texPath.C_Str());
			if (!fileName.empty())
			{
				materialRawData.diffuseMapFileName = TEXTURE_PATH_PREFIX + fileName;
			}
		}
		if (material->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS)
		{
			std::string fileName = GetFileNameFromPath(texPath.C_Str());
			if (!fileName.empty())
			{
				materialRawData.normalMapFileName = TEXTURE_PATH_PREFIX + fileName;
			}
		}
		if (material->GetTexture(aiTextureType_SPECULAR, 0, &texPath) == AI_SUCCESS)
		{
			std::string fileName = GetFileNameFromPath(texPath.C_Str());
			if (!fileName.empty())
			{
				materialRawData.specularMapFileName = TEXTURE_PATH_PREFIX + fileName;
			}
		}
	}

	// 3. Dùng MaterialManager để load material và lấy về index.
	uint32_t materialIndex = m_MaterialManager->LoadMaterial(materialRawData);
	
	// 4. Gán material index cho Mesh vừa tạo.
	newMesh->materialIndex = materialIndex;

	return newMesh;
}

std::string ModelLoader::GetFileNameFromPath(const std::string& fullPath)
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
