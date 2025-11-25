#pragma once

#include <string>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Scene/MaterialManager.h"

// Forward declarations
struct Vertex;
struct Mesh;
class MeshManager;
class MaterialManager;

// =================================================================================================
// Struct: MeshData
// Mô tả: Struct chứa dữ liệu thô của một mesh (đỉnh, chỉ số, và đường dẫn texture).
// =================================================================================================
struct MeshData
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	MaterialRawData materialRawData;
};

// =================================================================================================
// Class: ModelLoader
// Mô tả: Class tiện ích để tải dữ liệu model từ file bằng thư viện Assimp.
// =================================================================================================
class ModelLoader
{
public:
	ModelLoader(MeshManager* meshManager, MaterialManager* materialManager);
	~ModelLoader() = default;

	// Hàm chính để tải model từ file.
	// Trả về một vector các con trỏ tới Mesh đã được tạo và quản lý bởi MeshManager.
	std::vector<Mesh*> LoadModelFromFile(const std::string& filePath);

private:
	MeshManager* m_MeshManager;
	MaterialManager* m_MaterialManager;

	// Duyệt qua cây node của scene Assimp một cách đệ quy.
	void ProcessNode(aiNode* node, const aiScene* scene, std::vector<Mesh*>& outMeshes);
	
	// Xử lý một mesh đơn lẻ trong scene Assimp để trích xuất dữ liệu,
	// tạo Mesh và Material, và trả về con trỏ tới Mesh đã tạo.
	Mesh* ProcessMesh(aiMesh* mesh, const aiScene* scene);

	// Tiện ích để lấy tên file từ một đường dẫn đầy đủ.
	std::string GetFileNameFromPath(const std::string& fullPath);

	const std::string TEXTURE_PATH_PREFIX = "Resources/Textures/";
};