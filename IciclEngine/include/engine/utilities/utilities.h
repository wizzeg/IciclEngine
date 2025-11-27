#pragma once
#include <chrono>
#include <engine/utilities/macros.h>
#include <format>
#include <iostream>
#include <iomanip>

struct HighResolutionTimer
{
	using clock = std::chrono::high_resolution_clock;
	void start() { start_time = clock::now(); }
	void stop() { stop_time = clock::now(); }
	double get_time_ns() { return std::chrono::duration<double, std::nano>(stop_time - start_time).count(); }
	double get_time_us() { return std::chrono::duration<double, std::micro>(stop_time - start_time).count(); }
	double get_time_ms() { return std::chrono::duration<double, std::milli>(stop_time - start_time).count(); }
	double get_time_s() { return std::chrono::duration<double, std::ratio<1>>(stop_time - start_time).count(); }
private:
	clock::time_point start_time;
	clock::time_point stop_time;
};

struct TimeNow
{
    void print_time(const std::string& a_title) {
		auto now = std::chrono::system_clock::now();
		auto duration = now.time_since_epoch();

		auto hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();
		auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration % std::chrono::hours(1)).count();
		auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration % std::chrono::minutes(1)).count();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration % std::chrono::seconds(1)).count();
		auto us = std::chrono::duration_cast<std::chrono::microseconds>(duration % std::chrono::milliseconds(1)).count();
		auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration % std::chrono::microseconds(1)).count();

		PRINTLN("{} -- Time: {:02}h {:02}min {:02}s {:03}ms {:03}us {:03}ns",
			a_title, hours, minutes, seconds, ms, us, ns);
    }
};
