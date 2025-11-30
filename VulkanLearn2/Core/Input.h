#pragma once

class Input
{
public:
	Input() = delete;
	
	static void Init(GLFWwindow* window);
	
	static bool GetKey(int key);
	static bool GetMouseButton(int button);
	static glm::vec2 GetMousePosition();

	static void LockMouse(bool locked);
private:
	static GLFWwindow* s_Window;

	static glm::vec2 s_LastMoustPosition;
	static bool s_IsMouseLocked;
};
