#include "pch.h"
#include "Window.h"

Window::Window(int width, int height, const std::string& title):
	m_Width(width), 
	m_Height(height), 
	m_Title(title)
{
	// Khởi tạo thư viện GLFW.
	if (!glfwInit())
	{
		throw std::runtime_error("LỖI: Khởi tạo GLFW thất bại!");
	}

	// Cấu hình để GLFW không tự tạo OpenGL context, vì chúng ta sẽ dùng Vulkan.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	// Tạm thời không cho phép thay đổi kích thước cửa sổ.
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);
	//glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

	// Tạo cửa sổ GLFW.
	m_Window = glfwCreateWindow(m_Width, m_Height, m_Title.c_str(), nullptr, nullptr);
	glfwSetWindowSizeLimits(m_Window, m_Width, m_Height, m_Width, m_Height);

	if (!m_Window)
	{
		glfwTerminate();
		throw std::runtime_error("LỖI: Tạo cửa sổ GLFW thất bại!");
	}
}

Window::~Window()
{
	// Hủy cửa sổ và giải phóng tài nguyên GLFW.
	glfwDestroyWindow(m_Window);
	glfwTerminate();
}

bool Window::windowShouldClose()
{
	// Kiểm tra xem cửa sổ có nhận được yêu cầu đóng hay không (ví dụ: người dùng nhấn nút X).
	return glfwWindowShouldClose(m_Window);
}

void Window::windowPollEvents()
{
	// Xử lý các sự kiện của cửa sổ (như input từ bàn phím, chuột).
	glfwPollEvents();

	// Ví dụ: Đóng cửa sổ khi nhấn phím ESC.
	if (glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_Window, GLFW_TRUE);
	}
}

std::vector<const char*> Window::getInstanceExtensionsRequired()
{
	// Lấy danh sách các Vulkan instance extension mà GLFW cần để có thể hoạt động.
	uint32_t extensionCount = 0;
	const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
	
	std::vector<const char*> result(extensions, extensions + extensionCount);
	return result;
}

void Window::SetWindowTitle(const std::string& title)
{
	glfwSetWindowTitle(m_Window, title.c_str());
}
