#include "pch.h"
#include "Window.h"


Window::Window(int w, int h, const string& t):
	width(w), height(h), title(t)
{
	if (!glfwInit())
	{
		showError("FAILED TO INIT GLFW");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

	if (!window)
	{
		glfwTerminate();
		showError("FAILED TO CREATE GLFW WINDOW");
	}
	
}

bool Window::windowShouldClose()
{
	return glfwWindowShouldClose(window);
}

void Window::windowPollEvents()
{
	glfwPollEvents();
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}

Window::~Window()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

GLFWwindow* Window::getGLFWWindow()
{
	return window;
}

vector<const char*> Window::getInstanceExtensionsRequired()
{
	uint32_t extensionCount = 0;
	const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
	vector<const char*> result (extensions, extensions + extensionCount);
	return result;
}
