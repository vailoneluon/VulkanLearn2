#include "pch.h"
#include "Input.h"
#include "glm/gtx/norm.hpp"
#include "GameTime.h"


void Input::Init(GLFWwindow* window)
{
	s_Window = window;
	s_LastMousePosition = GetMousePosition();

	glfwSetCursorPosCallback(s_Window, MousePosCallBack);
}

void Input::Update()
{
	float smoothFactory = 0.0f;

	s_DeltaMousePosition = glm::mix(s_DeltaMousePosition, s_DeltaMousePositionAccumulator, 1 - smoothFactory);

	s_DeltaMousePositionAccumulator = { 0, 0 };
}

bool Input::GetKey(int key)
{
	int state = glfwGetKey(s_Window, key);
	return state == GLFW_PRESS && focusOnSceneViewPort;
}

bool Input::GetMouseButton(int button)
{
	int state = glfwGetMouseButton(s_Window, button);
	if (state == GLFW_PRESS && !hoverOnSceneViewPort) std::cout << "Not On Scene Viewport" << std::endl;
	return state == GLFW_PRESS && hoverOnSceneViewPort;
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
		//if (locked == false) std::cout << "UnLock" << std::endl;
		//glfwSetInputMode(s_Window, GLFW_RAW_MOUSE_MOTION, locked);

		s_IsMouseLocked = locked;
	}

	if (locked && s_FirstLocked)
	{
		s_FirstLocked = false;

		s_DeltaMousePosition = { 0, 0 };
		s_DeltaMousePositionAccumulator = { 0, 0 };
	}
	if (!locked && !s_FirstLocked)
	{
		s_FirstLocked = true;
	}
}

bool Input::hoverOnSceneViewPort = false;

bool Input::focusOnSceneViewPort = false;

void Input::MousePosCallBack(GLFWwindow* window, double xpos, double ypos)
{
	s_DeltaMousePositionAccumulator += glm::vec2(xpos - s_LastMousePosition.x, -(ypos - s_LastMousePosition.y));

	s_LastMousePosition = glm::vec2(xpos, ypos);
}

GLFWwindow* Input::s_Window;

bool Input::s_IsMouseLocked = false;

bool Input::s_FirstLocked = true;

glm::vec2 Input::s_LastMousePosition;

glm::vec2 Input::s_DeltaMousePosition(0, 0);

glm::vec2 Input::s_DeltaMousePositionAccumulator(0, 0);

