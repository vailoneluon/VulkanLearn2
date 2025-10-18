#pragma once
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <string>
#include <vector>

using namespace std;

class Window
{
public:
	Window(int width, int height, const string& title);
	~Window();

	GLFWwindow* getGLFWWindow();
	vector<const char*> getInstanceExtensionsRequired();

	// Không cho copy
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	
	bool windowShouldClose();
	void windowPollEvents();

private:
	int width;
	int height;
	string title;
	GLFWwindow* window;
};
