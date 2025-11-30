#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include "Core/Input.h"
#include "Scene.h"
#include "GLFW/glfw3.h"
#include "glm/gtx/norm.hpp"
#include "glm/glm.hpp"
#include "Core/GameTime.h"

class CameraControlSystem
{
public:
	static void CameraMoveUpdate(Scene* scene)
	{
		float MoveSpeed = 2.0f;
		glm::vec3 velocity = glm::vec3(0.0f);
		if (Input::GetKey(GLFW_KEY_W))
		{
			velocity.z += -1;
		}
		if (Input::GetKey(GLFW_KEY_S))
		{
			velocity.z += 1;
		}
		if (Input::GetKey(GLFW_KEY_A))
		{
			velocity.x += -1;
		}
		if (Input::GetKey(GLFW_KEY_D))
		{
			velocity.x += 1;
		}

		if (glm::length2(velocity) < 0.001f) return;

		auto view = scene->GetRegistry().view<TransformComponent, CameraComponent>();
		view.each([&](auto e, TransformComponent& transform, const CameraComponent& camera) 
			{
				if (!camera.IsPrimary()) return;
				transform.Translate(velocity * MoveSpeed * Core::Time::GetDeltaTime());
			});
	}
private:

};
