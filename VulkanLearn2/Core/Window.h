#pragma once

class Window
{
public:
	Window(int width, int height, const std::string& title);
	~Window();

	GLFWwindow* getGLFWWindow();
	std::vector<const char*> getInstanceExtensionsRequired();

	// Kh�ng cho copy
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	
	bool windowShouldClose();
	void windowPollEvents();

private:
	int width;
	int height;
	std::string title;
	GLFWwindow* window;
};
