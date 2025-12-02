#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct memory_checker
{
	static float get_mb_left()
	{
		MEMORYSTATUSEX statex;
		statex.dwLength = sizeof(statex);
		GlobalMemoryStatusEx(&statex);
		return static_cast<float>(statex.ullAvailPhys / (1024 * 1024));
	}

	static float get_b_left()
	{
		MEMORYSTATUSEX statex;
		statex.dwLength = sizeof(statex);
		GlobalMemoryStatusEx(&statex);
		return static_cast<float>(statex.ullAvailPhys);
	}
};
