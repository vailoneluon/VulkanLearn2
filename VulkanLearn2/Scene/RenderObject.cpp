#include "pch.h"
#include "RenderObject.h"
#include <Utils/ModelLoader.h>

RenderObject::RenderObject(const std::string& modelFilePath)
{
	LoadModelFromFile(modelFilePath);
}

RenderObject::~RenderObject()
{

}

void RenderObject::SetMeshRangeOffSet(VkDeviceSize vertexOffset /*= -1*/, VkDeviceSize indexOffset /*= -1*/)
{
	if (vertexOffset != -1)
	{
		handles.meshRange.vertexOffset = vertexOffset;
	}
	if (indexOffset != -1)
	{
		handles.meshRange.indexOffset = indexOffset;
		handles.meshRange.firstIndex = indexOffset / sizeof(handles.modelData.indices[0]);
	}
}

void RenderObject::Rotate(glm::vec3 angle)
{
	handles.transform.rotation += angle;
}

void RenderObject::Translate(glm::vec3 translation)
{
	handles.transform.position += translation;
}

void RenderObject::Scale(glm::vec3 scale)
{
	handles.transform.scale = scale;
}

glm::mat4 RenderObject::GetModelMatrix() const
{
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	
	modelMatrix = glm::translate(modelMatrix, handles.transform.position);

	modelMatrix = glm::rotate(modelMatrix, glm::radians(handles.transform.rotation.z), glm::vec3(0, 0, 1));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(handles.transform.rotation.y), glm::vec3(0, 1, 0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(handles.transform.rotation.x), glm::vec3(1, 0, 0));

	modelMatrix = glm::scale(modelMatrix, handles.transform.scale);

	return modelMatrix;
}

void RenderObject::LoadModelFromFile(const std::string& filePath)
{
	ModelLoader modelLoader;
	handles.modelData = modelLoader.LoadModelFromFile(filePath);

	handles.meshRange.vertexCount = handles.modelData.vertices.size();
	handles.meshRange.indexCount = handles.modelData.indices.size();
}
