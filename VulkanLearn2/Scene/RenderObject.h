#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

// Forward declarations
class Model;
class MeshManager;
class TextureManager;
class MaterialManager;

// =================================================================================================
// Struct: Transform
// Mô tả: Struct chứa các thông tin về vị trí, góc xoay, và tỷ lệ của một đối tượng.
// =================================================================================================
struct Transform
{
	glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 rotation = glm::vec3(0.0f, 0.0f, 0.0f); // In degrees
	glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
};

// =================================================================================================
// Struct: RenderObjectHandles
// Mô tả: Struct chứa các handle và dữ liệu nội bộ của một RenderObject.
// =================================================================================================
struct RenderObjectHandles
{
	Transform transform;
	Model* model = nullptr;
};

// =================================================================================================
// Class: RenderObject
// Mô tả: 
//      Đại diện cho một đối tượng có thể được vẽ trong scene.
//      Bao gồm model (mesh + material) và thông tin biến đổi (transform).
// =================================================================================================
class RenderObject
{
public:
	// Constructor: Tạo RenderObject từ file model.
	RenderObject(const std::string& modelFilePath, MeshManager* meshManager, MaterialManager* materialManager);
	~RenderObject();

	// --- Thao tác với Transform ---
	
	// Xoay đối tượng thêm một góc (độ).
	void Rotate(glm::vec3 angle);
	
	// Di chuyển đối tượng thêm một khoảng.
	void Translate(glm::vec3 translation);
	
	// Thay đổi tỷ lệ của đối tượng.
	void Scale(glm::vec3 scale);

	// Thiết lập vị trí tuyệt đối.
	void SetPosition(glm::vec3 newPosition);
	
	// Thiết lập góc xoay tuyệt đối.
	void SetRotation(glm::vec3 newRotation);

	// --- Getters ---
	
	// Tính toán và trả về ma trận model dựa trên transform hiện tại.
	glm::mat4 GetModelMatrix() const;
	
	// Lấy các handle nội bộ.
	const RenderObjectHandles& getHandles() const { return m_Handles; }

private:
	RenderObjectHandles m_Handles;
};
