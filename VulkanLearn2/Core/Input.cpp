#include "pch.h"
#include "Input.h"

void Input::Init(GLFWwindow* window)
{
	s_Window = window;
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

GLFWwindow* Input::s_Window;
