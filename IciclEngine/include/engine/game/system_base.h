#pragma once
#include <engine/core/systems_context.h>
struct SystemBase
{
	virtual bool execute(SystemsContext& ctx) = 0;
	const std::string& get_name() const { return name; }
	void set_name(const std::string& a_name) { name = a_name; }
	uint32_t get_order() const { return order; }
	void set_order(const uint32_t a_order) { order = a_order; }
	uint32_t order = 5000;
	void set_enabled(bool a_enable) { enabled = a_enable; }
	bool get_enabled() const { return enabled; };
	void set_only_on_physics(bool a_physics_only) { only_physics_frames = a_physics_only; }
	bool get_physics_frames_only() const { return only_physics_frames; }

	bool operator < (const SystemBase& other) const
	{
		return order < other.order;
	}
private:
	std::string name = "SystemBase";
	bool enabled = true;
	bool only_physics_frames = false;
};