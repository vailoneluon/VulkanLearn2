#pragma once

struct RenderObjectHandles 
{
	ModelData modelData;
	ModelTransform transform;
	MeshRange meshRange;
};

class RenderObject
{
public:
	RenderObject(const std::string& modelFilePath);
	~RenderObject();

	const RenderObjectHandles& getHandles() const { return handles; }

	void SetMeshRangeOffSet(VkDeviceSize vertexOffset = -1, VkDeviceSize indexOffset = -1);

	void Rotate(glm::vec3 angle);
	void Translate(glm::vec3 translation);
	void Scale(glm::vec3 scale);

	void SetPosition(glm::vec3 position);

	glm::mat4 GetModelMatrix() const;
private:
	RenderObjectHandles handles;

	void LoadModelFromFile(const std::string& filePath);
};
