#pragma once

// Giữ các include này để file .cpp có thể truy cập
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct MeshData
{
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	std::string textureFilePath;
};

struct ModelData
{
	std::vector<MeshData> meshData;
};

class ModelLoader
{
public:
	ModelLoader();
	~ModelLoader();

	// Hàm này sẽ làm tất cả công việc
	ModelData LoadModelFromFile(const std::string& filePath);

private:
	// Không cần hàm private hay biến thành viên nữa
};