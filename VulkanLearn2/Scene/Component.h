#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "LightData.h"

class Model;

struct TransformComponent 
{
	friend class TransformSystem;
public:

	glm::vec3 GetPosition()			const	{ return m_Position; }
	glm::vec3 GetRotation()			const	{ return m_Rotation; }
	glm::vec3 GetScale()			const	{ return m_Scale; }
	glm::mat4 GetTransformMatrix()	const	{ return m_TransformMatrix; }

	void SetPosition(glm::vec3 pos)			{ m_Position = pos;		m_IsDirty = true; }
	void SetRotation(glm::vec3 rotation)	{ m_Rotation = rotation;	m_IsDirty = true; }
	void SetScale(glm::vec3 scale)			{ m_Scale = scale;		m_IsDirty = true; }

	void Translate(glm::vec3 positionDelta) { m_Position += positionDelta;	m_IsDirty = true; }
	void Rotate(glm::vec3 rotateDelta)		{ m_Rotation += rotateDelta;		m_IsDirty = true; }

private:
	glm::vec3 m_Position{ 0.0f };
	glm::vec3 m_Rotation{ 0.0f };
	glm::vec3 m_Scale{ 1.0f };

	mutable	bool m_IsDirty = true;
	mutable glm::mat4 m_TransformMatrix = glm::mat4(1.0f);
};

struct MeshComponent
{
	Model* Model = nullptr;
	bool IsVisible = true;
};

struct NameComponent
{
	std::string Name;
};

struct LightComponent
{
	Light Data;
	bool IsEnable = true;
};

struct CameraComponent
{
	float Fov = 45.0f;
	float Near = 0.01f;
	float Far = 1000.0f;
	float AspectRatio = 1.777f;

	bool IsPrimary = true;

	glm::mat4 GetProjectionMatrix() const
	{
		glm::mat4 proj = glm::perspective(Fov, AspectRatio, Near, Far);
		proj[1][1] *= -1;

		return proj;
	}
};