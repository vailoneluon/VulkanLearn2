#pragma once

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
