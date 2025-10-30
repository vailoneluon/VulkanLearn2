#include "pch.h"
#include "ModelLoader.h"
#include "Core/VulkanContext.h" // Include để có định nghĩa đầy đủ của struct Vertex
#include <stdexcept>

// --- Các hàm helper xử lý dữ liệu Assimp --- 
// Được đặt trong anonymous namespace để giới hạn phạm vi truy cập chỉ trong file này.
namespace {

	// Duyệt qua cây node của scene Assimp một cách đệ quy.
	void ProcessNode(aiNode* node, const aiScene* scene, ModelData& modelData);

	// Xử lý một mesh đơn lẻ trong scene Assimp để trích xuất dữ liệu.
	MeshData ProcessMesh(aiMesh* mesh, const aiScene* scene);

	// Tiện ích để lấy tên file từ một đường dẫn đầy đủ.
	std::string GetFileNameFromPath(const std::string& fullPath);

} // anonymous namespace


// --- Triển khai các hàm của ModelLoader ---

ModelData ModelLoader::LoadModelFromFile(const std::string& filePath)
{
	ModelData modelData;
	Assimp::Importer importer;

	// Các cờ xử lý hậu kỳ của Assimp.
	// Yêu cầu Assimp thực hiện các bước chuẩn hóa dữ liệu sau khi tải.
	unsigned int flags = 
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_GenSmoothNormals |
		//aiProcess_FlipUVs |
		aiProcess_CalcTangentSpace;

	// LƯU Ý: Đoạn code này kiểm tra nếu file là .assbin (định dạng nhị phân của Assimp)
	// thì sẽ bỏ qua tất cả các cờ xử lý hậu kỳ. Điều này giả định rằng file .assbin
	// đã được xử lý trước đó.
	if (filePath.size() > 7 && filePath.substr(filePath.size() - 7) == ".assbin")
	{
		flags = 0;
	}

	// Đọc file model.
	const aiScene* scene = importer.ReadFile(filePath, flags);

	// Kiểm tra lỗi.
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		throw std::runtime_error("LỖI ASSIMP: " + std::string(importer.GetErrorString()));
	}

	// Bắt đầu xử lý từ node gốc của scene.
	ProcessNode(scene->mRootNode, scene, modelData);

	return modelData;
}


// --- Định nghĩa các hàm helper trong anonymous namespace ---
namespace {

	void ProcessNode(aiNode* node, const aiScene* scene, ModelData& modelData)
	{
		// Xử lý tất cả các mesh trong node hiện tại.
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			modelData.meshData.push_back(ProcessMesh(mesh, scene));
		}

		// Duyệt đệ quy qua tất cả các node con.
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], scene, modelData);
		}
	}

	MeshData ProcessMesh(aiMesh* mesh, const aiScene* scene)
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

			// Assimp hỗ trợ nhiều bộ UV, ở đây ta chỉ lấy bộ đầu tiên (kênh 0).
			if (mesh->mTextureCoords[0])
			{
				vertex.uv = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
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

		// Trích xuất đường dẫn file texture từ material.
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		if (material)
		{
			aiString texPath;
			// Chỉ lấy texture khuếch tán (diffuse) đầu tiên.
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
			{
				currentMeshData.textureFilePath = GetFileNameFromPath(texPath.C_Str());
			}
		}

		return currentMeshData;
	}

	std::string GetFileNameFromPath(const std::string& fullPath)
	{
		// Tìm vị trí dấu gạch chéo cuối cùng.
		size_t lastSlash = fullPath.find_last_of("/\\");

		if (lastSlash == std::string::npos)
		{
			return fullPath; // Không có dấu gạch chéo, trả về toàn bộ chuỗi.
		}
		else
		{
			return fullPath.substr(lastSlash + 1); // Trả về chuỗi con sau dấu gạch chéo.
		}
	}

} // anonymous namespace