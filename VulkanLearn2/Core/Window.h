#pragma once

#include <string>
#include <vector>

// =================================================================================================
// Class: Window
// Mô tả: 
//      Đóng gói một cửa sổ GLFW.
//      Chịu trách nhiệm khởi tạo, quản lý vòng đời và các tương tác cơ bản với cửa sổ.
// =================================================================================================
class Window
{
public:
	// Constructor: Tạo cửa sổ mới với kích thước và tiêu đề cho trước.
	Window(int width, int height, const std::string& title);
	~Window();

	// Không cho phép copy hoặc gán để tránh quản lý sai tài nguyên cửa sổ.
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	// --- Getters ---
	GLFWwindow* getGLFWWindow() const { return m_Window; }
	
	// Lấy danh sách các Vulkan instance extension mà GLFW yêu cầu.
	std::vector<const char*> getInstanceExtensionsRequired();

	// --- Tương tác với cửa sổ ---
	
	void SetWindowTitle(const std::string& title);

	// Kiểm tra xem cửa sổ có nên đóng hay không.
	bool windowShouldClose();
	
	// Xử lý các sự kiện input.
	void windowPollEvents();

private:
	// --- Dữ liệu nội bộ ---
	int m_Width;
	int m_Height;
	std::string m_Title;
	GLFWwindow* m_Window;
};