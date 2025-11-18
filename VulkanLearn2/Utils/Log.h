#pragma once
#include <string>
#include <iostream>

namespace Log
{
	// ANSI escape codes for colors
	const char* const RESET_COLOR = "\033[0m";
	const char* const RED_COLOR = "\033[31m";
	const char* const YELLOW_COLOR = "\033[33m";
	const char* const GREEN_COLOR = "\033[32m";
	const char* const PINK_COLOR = "\033[38;2;255;105;180m"; // For the message body

	inline void Info(const std::string& message)
	{
		std::cout << GREEN_COLOR << "[INFO] " << RESET_COLOR << message << std::endl;
	}

	inline void Warning(const std::string& message)
	{
		std::cerr << YELLOW_COLOR << "[WARNING] " << PINK_COLOR << message << RESET_COLOR << std::endl;
	}

	inline void Error(const std::string& message)
	{
		std::cerr << RED_COLOR << "[ERROR] " << PINK_COLOR << message << RESET_COLOR << std::endl;
	}
}
