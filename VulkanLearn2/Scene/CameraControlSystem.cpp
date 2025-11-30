#include "pch.h"
#include "CameraControlSystem.h"
#include "Component.h"
#include "GLFW/glfw3.h"
#include "glm/gtx/norm.hpp"
#include "glm/glm.hpp"
#include "Core/GameTime.h"

void CameraControlSystem::CamereTransformUpdate(Scene* scene)
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

void CameraControlSystem::CameraRotateUpdate(Scene* scene)
{
	if (Input::GetMouseButton(GLFW_MOUSE_BUTTON_RIGHT))
	{
		Input::LockMouse(true);
		if (s_IsFirstRotate)
		{
			s_LastMousePosition = Input::GetMousePosition();
			s_IsFirstRotate = false;
			return;
		}
		else
		{
			glm::vec2 currentMousePos = Input::GetMousePosition();
			glm::vec2 deltaMousePosition = currentMousePos - s_LastMousePosition;
			s_LastMousePosition = currentMousePos;

			if (glm::length(deltaMousePosition) != 0)
			{
				auto view = scene->GetRegistry().view<TransformComponent, CameraComponent>();
				view.each([&](auto e, TransformComponent& transform, const CameraComponent& camera)
					{
						if (!camera.IsPrimary()) return;
						transform.Rotate({ -deltaMousePosition.y * s_RotateSpeed, deltaMousePosition.x * s_RotateSpeed, 0 });
					});
			}
		}
	}
	else
	{
			Input::LockMouse(false);
			s_IsFirstRotate = true;
	}

}

glm::vec2 CameraControlSystem::s_LastMousePosition = glm::vec2(0);


bool CameraControlSystem::s_IsFirstRotate = true;

const float CameraControlSystem::s_RotateSpeed = 0.2f;

