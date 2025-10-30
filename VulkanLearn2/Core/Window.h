#pragma once

#include <string>
#include <vector>

// Class đóng gói một cửa sổ GLFW.
// Chịu trách nhiệm khởi tạo, quản lý vòng đời và các tương tác cơ bản với cửa sổ.
class Window
{
public:
	Window(int width, int height, const std::string& title);
	~Window();

	// Không cho phép copy hoặc gán để tránh quản lý sai tài nguyên cửa sổ.
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	// --- Getters ---
	GLFWwindow* getGLFWWindow() const { return m_Window; }
	std::vector<const char*> getInstanceExtensionsRequired();

	// --- Tương tác với cửa sổ ---
	bool windowShouldClose();
	void windowPollEvents();

private:
	// --- Dữ liệu nội bộ ---
	int m_Width;
	int m_Height;
	std::string m_Title;
	GLFWwindow* m_Window;
};