#pragma once

// === Các thư viện chuẩn (STL) ===
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
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

// 3. STB Image
#include "stb_image.h"

// 4. VMA
#include "vma/vk_mem_alloc.h"

// === Các file header nội bộ ổn định ===
#include "Utils/ErrorHelper.h"
#include "Core/VulkanTypes.h" // Thêm cả VulkanTypes vào PCH

