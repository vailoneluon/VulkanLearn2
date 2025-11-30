#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include "Core/Input.h"
#include "Scene.h"


class CameraControlSystem
{
public:
	static void CamereTransformUpdate(Scene* scene);

	static void CameraRotateUpdate(Scene* scene);
private:
	static glm::vec2 s_LastMousePosition;
	static bool s_IsFirstRotate;

	static const float s_RotateSpeed;
};
