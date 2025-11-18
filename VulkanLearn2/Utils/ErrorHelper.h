#pragma once
#include <stdexcept>
#include <string>
#include <sstream>
#include <iostream>

#ifdef _MSC_VER
#define DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#define DEBUG_BREAK() __builtin_trap()
#else
#define DEBUG_BREAK() std::abort()
#endif

class VulkanError : public std::runtime_error
{
public:
	VulkanError(const std::string& msg, const char* file, int line)
		: std::runtime_error(FormatMessage(msg, file, line))
	{
		Log::Error(what());
#ifdef _DEBUG
		DEBUG_BREAK(); // Break ngay khi throw (debug mode)
#endif
	}

private:
	static std::string FormatMessage(const std::string& msg, const char* file, int line)
	{
		std::ostringstream oss;
		oss << msg << "\nAt: " << file << ":" << line;
		return oss.str();
	}
};

#define showError(msg) throw VulkanError(msg, __FILE__, __LINE__)

// Wrapper cho Vulkan result check
#define VK_CHECK(result, msg) \
    if (result != VK_SUCCESS) { \
        showError(std::string(msg) + " (VkResult: " + std::to_string(result) + ")"); \
    }