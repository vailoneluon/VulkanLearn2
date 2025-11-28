#pragma once
#include "Scene.h"
#include "Component.h"

class TransformSystem
{
public:
	static void UpdateTransformMatrix(Scene* scene)
	{
		auto view = scene->GetRegistry().view<TransformComponent>();

		view.each([](auto e, const TransformComponent& transform)
			{
				if (transform.m_IsDirty)
				{
					UpdateTransformMatrix(transform);
					transform.m_IsDirty = false;
				}
			});
	}

	static glm::mat4 GetViewMatrix(const TransformComponent& transform)
	{
		float yaw = transform.m_Rotation.y;
		float pitch = transform.m_Rotation.x;

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
		glm::mat4 viewMatrix = glm::lookAt(transform.m_Position, transform.m_Position + cameraFront, worldUp);

		return viewMatrix;
	}

private:
	static void UpdateTransformMatrix(const TransformComponent& transform)
	{
		glm::mat4 mat = glm::mat4(1.0f);
		mat = glm::translate(mat, transform.m_Position);
		mat = glm::rotate(mat, glm::radians(transform.m_Rotation.z), { 0,0,1 });
		mat = glm::rotate(mat, glm::radians(transform.m_Rotation.y), { 0,1,0 });
		mat = glm::rotate(mat, glm::radians(transform.m_Rotation.x), { 1,0,0 });
		mat = glm::scale(mat, transform.m_Scale);

		transform.m_TransformMatrix = mat;
	}
};