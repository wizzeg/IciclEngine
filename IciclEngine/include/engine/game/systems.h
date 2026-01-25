#pragma once
#include <engine/game/system_base.h>

struct MoveSystem : SystemBase
{
	double time = 0;
	bool execute(SystemsContext& ctx) override;
	//const std::string& get_name() override { return "MoveSystem"; }
};

struct TransformCalculationSystem : SystemBase
{
	bool execute(SystemsContext& ctx) override;
	//const std::string& get_name() override { return "TransformCalculationSystem"; }
};