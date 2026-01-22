#pragma once
#include <engine/game/system_base.h>

struct MoveSystem : SystemBase
{
	double time = 0;
	void execute(SystemsContext& ctx) override;
};

struct TransformCalculationSystem : SystemBase
{
	void execute(SystemsContext& ctx) override;
};