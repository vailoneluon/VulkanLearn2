#include "pch.h"
#include "RenderObject.h"
#include "Model.h"
#include "MaterialManager.h"

RenderObject::RenderObject(const std::string& modelFilePath, MeshManager* meshManager, MaterialManager* materialManager)
{
	// RenderObject sở hữu một đối tượng Model, được tạo ra khi RenderObject được tạo.
	m_Handles.model = new Model(modelFilePath, meshManager, materialManager);
}

RenderObject::~RenderObject()
{
	// Giải phóng đối tượng Model khi RenderObject bị hủy.
	delete(m_Handles.model);
}

void RenderObject::Rotate(glm::vec3 angle)
{
	// Cộng dồn góc xoay.
	m_Handles.transform.rotation += angle;
}

void RenderObject::Translate(glm::vec3 translation)
{
	// Cộng dồn vào vị trí.
	m_Handles.transform.position += translation;
}

void RenderObject::Scale(glm::vec3 scale)
{
	// Thiết lập tỷ lệ mới (thường là ghi đè, không cộng dồn).
	m_Handles.transform.scale = scale;
}

void RenderObject::SetPosition(glm::vec3 newPosition)
{
	// Thiết lập vị trí mới.
	m_Handles.transform.position = newPosition;
}

void RenderObject::SetRotation(glm::vec3 newRotation)
{
	// Thiết lập góc xoay mới.
	m_Handles.transform.rotation = newRotation;
}

glm::mat4 RenderObject::GetModelMatrix() const
{
	// Bắt đầu với ma trận đơn vị.
	glm::mat4 modelMatrix = glm::mat4(1.0f);

	// 1. Áp dụng phép tịnh tiến (Translate)
	modelMatrix = glm::translate(modelMatrix, m_Handles.transform.position);

	// 2. Áp dụng các phép xoay (Rotate)
	// Thứ tự xoay có thể quan trọng, ở đây là Z-Y-X.
	modelMatrix = glm::rotate(modelMatrix, glm::radians(m_Handles.transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(m_Handles.transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(m_Handles.transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));

	// 3. Áp dụng phép co giãn (Scale)
	modelMatrix = glm::scale(modelMatrix, m_Handles.transform.scale);

	return modelMatrix;
}