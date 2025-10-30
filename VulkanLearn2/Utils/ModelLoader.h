#pragma once

#include <string>
#include <vector>

// LƯU Ý: Các include của Assimp được đặt ở đây để file .cpp có thể sử dụng,
// nhưng chúng làm cho header này phụ thuộc vào thư viện Assimp.
// Một thiết kế tốt hơn có thể là dùng forward declaration cho các kiểu của Assimp
// và chỉ include trong file .cpp (PIMPL idiom).
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Forward-declare struct Vertex để tránh include file VulkanContext.h không cần thiết.
struct Vertex;

// Struct chứa dữ liệu thô của một mesh (đỉnh, chỉ số, và đường dẫn texture).
struct MeshData
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::string textureFilePath;
};

// Struct chứa dữ liệu của toàn bộ model, bao gồm một hoặc nhiều MeshData.
struct ModelData
{
	std::vector<MeshData> meshData;
};

// Class tiện ích để tải dữ liệu model từ file bằng thư viện Assimp.
// Class này không có trạng thái, hoạt động như một namespace chứa hàm.
class ModelLoader
{
public:
	ModelLoader() = default;
	~ModelLoader() = default;

	// Hàm chính để tải model từ file.
	// Trả về một struct ModelData chứa tất cả dữ liệu đã được xử lý.
	ModelData LoadModelFromFile(const std::string& filePath);

private:
	// Class này không có biến thành viên.
};
