#include "pch.h"
#include "Input.h"
#include "glm/gtx/norm.hpp"
#include "GameTime.h"


void Input::Init(GLFWwindow* window)
{
	s_Window = window;
	s_LastMousePosition = GetMousePosition();
}

void Input::Update()
{
	glm::vec2 currentMousePosition = GetMousePosition();
	glm::vec2 rawDelta = currentMousePosition - s_LastMousePosition;

	rawDelta.y *= -1;

	float smoothFactory = 0.5f;
	if (glm::length2(rawDelta) >= 0.01f)
	{
		s_DeltaMousePosition = glm::mix(s_DeltaMousePosition, rawDelta, smoothFactory);
	}
	else
	{
		s_DeltaMousePosition = glm::vec2(0, 0);
	}

	s_LastMousePosition = currentMousePosition;
}

bool Input::GetKey(int key)
{
	int state = glfwGetKey(s_Window, key);
	return state == GLFW_PRESS;
}

bool Input::GetMouseButton(int button)
{
	int state = glfwGetMouseButton(s_Window, button);
	return state == GLFW_PRESS;
}

glm::vec2 Input::GetMousePosition()
{
	double xpos, ypos;
	glfwGetCursorPos(s_Window, &xpos, &ypos);
	return { (float)xpos, (float)ypos };
}

glm::vec2 Input::GetDeltaMousePosition()
{
	return s_DeltaMousePosition;
}

void Input::LockMouse(bool locked)
{
	if (s_IsMouseLocked != locked)
	{
		int cursorMode = locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
		glfwSetInputMode(s_Window, GLFW_CURSOR, cursorMode);
		glfwSetInputMode(s_Window, GLFW_RAW_MOUSE_MOTION, locked);

		s_IsMouseLocked = locked;
	}
}

GLFWwindow* Input::s_Window;

bool Input::s_IsMouseLocked = false;

glm::vec2 Input::s_LastMousePosition;

glm::vec2 Input::s_DeltaMousePosition;

