#define GLM_ENABLE_EXPERIMENTAL
#pragma once

class Input
{
public:
	Input() = delete;
	
	static void Init(GLFWwindow* window);

	static void Update();
	
	static bool GetKey(int key);
	static bool GetMouseButton(int button);
	static glm::vec2 GetMousePosition();
	static glm::vec2 GetDeltaMousePosition();

	static void LockMouse(bool locked);
private:
	static GLFWwindow* s_Window;

	static bool s_IsMouseLocked;

	static glm::vec2 s_LastMousePosition;
	static glm::vec2 s_DeltaMousePosition;
};
