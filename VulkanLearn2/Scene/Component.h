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

	glm::mat4 GetTransformMatrix() const
	{
		glm::mat4 mat = glm::mat4(1.0f);
		mat = glm::translate(mat, Position);
		mat = glm::rotate(mat, glm::radians(Rotation.z), { 0,0,1 });
		mat = glm::rotate(mat, glm::radians(Rotation.y), { 0,1,0 });
		mat = glm::rotate(mat, glm::radians(Rotation.x), { 1,0,0 });
		mat = glm::scale(mat, Scale);
		return mat;
	}

	glm::mat4 GetViewMatrix() const
	{
		float yaw = Rotation.y;
		float pitch = Rotation.x;

		glm::vec3 front;
		front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		front.y = sin(glm::radians(pitch));
		front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

		// Chuẩn hóa vector để đảm bảo độ dài bằng 1
		glm::vec3 cameraFront = glm::normalize(front);

		// 2. Định nghĩa trục Up của thế giới (thường là trục Y dương)
		glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

		// 3. Tính View Matrix bằng glm::lookAt
		// Tham số 2 (Target) chính là: Vị trí Camera + Hướng nhìn
		glm::mat4 viewMatrix = glm::lookAt(Position, Position + cameraFront, worldUp);

		return viewMatrix;
	}
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