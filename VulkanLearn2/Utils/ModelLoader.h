#pragma once

// Giữ các include này để file .cpp có thể truy cập
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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