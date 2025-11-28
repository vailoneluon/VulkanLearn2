#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "LightData.h"

class Model;

struct TransformComponent 
{
	glm::vec3 Position{ 0.0f };
	glm::vec3 Rotation{ 0.0f };
	glm::vec3 Scale{ 1.0f };
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

static glm::mat4 GetTransformMatrix(const TransformComponent& transform)
{
	glm::mat4 mat = glm::mat4(1.0f);
	mat = glm::translate(mat, transform.Position);
	mat = glm::rotate(mat, glm::radians(transform.Rotation.z), { 0,0,1 });
	mat = glm::rotate(mat, glm::radians(transform.Rotation.y), { 0,1,0 });
	mat = glm::rotate(mat, glm::radians(transform.Rotation.x), { 1,0,0 });
	mat = glm::scale(mat, transform.Scale);
	return mat;
}