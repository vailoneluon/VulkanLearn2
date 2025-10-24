#pragma once

// === Các thư viện chuẩn (STL) ===
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <array>
#include <unordered_map>
#include <stdexcept>
#include <algorithm>
#include <chrono>

// === Các thư viện bên ngoài (External) ===

// 1. GLFW (Bao gồm cả Vulkan)
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

// 2. GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

// 3. STB Image (Chỉ include header, không implementation)
#include "stb_image.h"

// === Các file header nội bộ ổn định ===
// ErrorHelper.h là một ứng viên tốt vì nó được dùng ở mọi nơi
#include "../Utils/ErrorHelper.h"