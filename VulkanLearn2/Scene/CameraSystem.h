#pragma once

#include "Scene.h"
#include "Component.h"

class CameraSystem
{
public:
	static void UpdateCameraMatrix(Scene* scene)
	{
		auto view = scene->GetRegistry().view<TransformComponent, CameraComponent>();
		view.each([](auto e, const TransformComponent& transform, const CameraComponent& camera) 
			{
				if (camera.m_IsProjDirty)
				{
					UpdateProjectionMatrix(camera);
					camera.m_IsProjDirty = false;
				}

				UpdateViewMatrix(transform, camera);
			});
	}
private:
	static void UpdateProjectionMatrix(const CameraComponent& camera)
	{
		glm::mat4 proj = glm::perspective(camera.m_Fov, camera.m_AspectRatio, camera.m_Near, camera.m_Far);
		proj[1][1] *= -1;

		camera.m_ProjMatrix = proj;
	}

	static void UpdateViewMatrix(const TransformComponent& transform, const CameraComponent& camera)
	{
		float yaw = transform.GetRotation().y;
		float pitch = transform.GetRotation().x;

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
		glm::mat4 viewMatrix = glm::lookAt(transform.GetPosition(), transform.GetPosition() + cameraFront, worldUp);

		camera.m_ViewMatrix = viewMatrix;
	}
};
