#pragma once
class Model;
class MeshManager;
class TextureManager;

struct Transform
{
	glm::vec3 position = glm::vec3(0, 0, 0);
	glm::vec3 rotation = glm::vec3(0, 0, 0);
	glm::vec3 scale = glm::vec3(1, 1, 1);
};


struct RenderObjectHandles
{
	Transform transform;
	Model* model;
};

class RenderObject
{
public:
	RenderObject(const std::string& modelFilePath, MeshManager* meshManager, TextureManager* textureManager);
	~RenderObject();

	void Rotate(glm::vec3 angle);
	void Translate(glm::vec3 translation);
	void Scale(glm::vec3 scale);

	void SetPosition(glm::vec3 newPosition);
	void SetRotation(glm::vec3 newRotation);

	glm::mat4 GetModelMatrix() const;
	const RenderObjectHandles& getHandles() const { return handles; }

private:
	RenderObjectHandles handles;


};