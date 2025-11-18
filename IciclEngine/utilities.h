#pragma once
#include <chrono>
#include "macros.h"

struct HighResolutionTimer
{
	using clock = std::chrono::high_resolution_clock;
	void start() { start_time = clock::now(); }
	void stop() { stop_time = clock::now(); }
	double get_time_ns() { return std::chrono::duration<double, std::nano>(stop_time - start_time).count(); }
	double get_time_us() { return std::chrono::duration<double, std::micro>(stop_time - start_time).count(); }
	double get_time_ms() { return std::chrono::duration<double, std::milli>(stop_time - start_time).count(); }
private:
	clock::time_point start_time;
	clock::time_point stop_time;
};