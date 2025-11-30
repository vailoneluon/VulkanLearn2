#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include "Core/Input.h"
#include "Scene.h"


class CameraControlSystem
{
public:
	static void CameraTransformUpdate(Scene* scene);

	static void CameraRotateUpdate(Scene* scene);
private:
	static const float s_RotateSpeed;
};
