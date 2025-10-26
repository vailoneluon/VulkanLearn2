#pragma once
#include <iostream>
#include <chrono>
#include <string>

class ScopeTimer
{
public:
	ScopeTimer(const std::string& name)
		: m_Name(name), m_StartTime(std::chrono::high_resolution_clock::now())
	{
	}

	~ScopeTimer()
	{
		auto endTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> duration = endTime - m_StartTime;
		std::cout << "[TIMER] " << m_Name << " took: " << duration.count() << " ms\n";
	}

private:
	std::string m_Name;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
};