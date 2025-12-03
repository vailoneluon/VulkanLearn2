#include "pch.h"
#include "CameraControlSystem.h"
#include "Component.h"
#include "GLFW/glfw3.h"
#include "glm/gtx/norm.hpp"
#include "glm/glm.hpp"
#include "Core/GameTime.h"

void CameraControlSystem::CameraTransformUpdate(Scene* scene)
{
	float MoveSpeed = 2.0f;
	glm::vec3 UP{ 0, 1, 0 };
	
	auto view = scene->GetRegistry().view<TransformComponent, CameraComponent>();
	view.each([&](auto e, TransformComponent& transform, const CameraComponent& camera)
		{
			if (!camera.IsPrimary()) return;

			glm::vec3 velocity = glm::vec3(0.0f);

			if (Input::GetKey(GLFW_KEY_W))
			{
				velocity += transform.GetForward();
			}
			if (Input::GetKey(GLFW_KEY_S))
			{
				velocity -= transform.GetForward();
			}
			if (Input::GetKey(GLFW_KEY_A))
			{
				velocity -= transform.GetRight();
			}
			if (Input::GetKey(GLFW_KEY_D))
			{
				velocity += transform.GetRight();
			}
			if (Input::GetKey(GLFW_KEY_E) || Input::GetKey(GLFW_KEY_SPACE))
			{
				velocity += UP;
			}
			if (Input::GetKey(GLFW_KEY_Q) || Input::GetKey(GLFW_KEY_LEFT_SHIFT))
			{
				velocity -= UP;
			}
			if (glm::length2(velocity) < 0.01f) return;

			velocity = glm::normalize(velocity);

			transform.Translate(velocity * MoveSpeed * Core::Time::GetDeltaTime());
		});
}

void CameraControlSystem::CameraRotateUpdate(Scene* scene)
{
	if (Input::GetMouseButton(GLFW_MOUSE_BUTTON_LEFT))
	{
		Input::LockMouse(true);
		glm::vec2 deltaMousePosition = Input::GetDeltaMousePosition();
		if (glm::length(deltaMousePosition) != 0)
		{
			auto view = scene->GetRegistry().view<TransformComponent, CameraComponent>();
			for (auto [entity, transform, camera] : view.each())
			{	
				if (!camera.IsPrimary()) continue;
				transform.Rotate({ deltaMousePosition.y * s_RotateSpeed, deltaMousePosition.x * s_RotateSpeed, 0 });

				glm::vec3 currentRotation = transform.GetRotation();
				currentRotation.x = std::clamp(currentRotation.x, -89.0f, 89.0f);
				transform.SetRotation(currentRotation);
			};
		}
	}
	else
	{
		Input::LockMouse(false);

	}

}

const float CameraControlSystem::s_RotateSpeed = 0.07f;

