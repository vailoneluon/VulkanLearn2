#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

// Forward declarations
class Model;
class MeshManager;
class TextureManager;

// Struct chứa các thông tin về vị trí, góc xoay, và tỷ lệ của một đối tượng.
struct Transform
{
	glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 rotation = glm::vec3(0.0f, 0.0f, 0.0f); // In degrees
	glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
};


// Struct chứa các handle và dữ liệu nội bộ của một RenderObject.
struct RenderObjectHandles
{
	Transform transform;
	Model* model = nullptr;
};

// Class đại diện cho một đối tượng có thể được vẽ trong scene.
// Nó bao gồm một Model (dữ liệu hình học) và một Transform (vị trí trong không gian).
class RenderObject
{
public:
	RenderObject(const std::string& modelFilePath, MeshManager* meshManager, TextureManager* textureManager);
	~RenderObject();

	// --- Thao tác với Transform ---
	void Rotate(glm::vec3 angle);
	void Translate(glm::vec3 translation);
	void Scale(glm::vec3 scale);

	void SetPosition(glm::vec3 newPosition);
	void SetRotation(glm::vec3 newRotation);

	// --- Getters ---
	// Tính toán và trả về ma trận model dựa trên transform hiện tại.
	glm::mat4 GetModelMatrix() const;
	const RenderObjectHandles& getHandles() const { return m_Handles; }

private:
	RenderObjectHandles m_Handles;
};
