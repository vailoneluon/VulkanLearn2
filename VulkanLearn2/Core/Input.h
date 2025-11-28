#pragma once

class Input
{
public:
	Input() = delete;
	
	static void Init(GLFWwindow* window);
	
	static bool GetKey(int key);
	static bool GetMouseButton(int button);
	static glm::vec2 GetMousePosition();
private:
	static GLFWwindow* s_Window;

};
