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
	std::string name = "SystemBase";

	bool operator < (const SystemBase& other) const
	{
		return order < other.order;
	}
};