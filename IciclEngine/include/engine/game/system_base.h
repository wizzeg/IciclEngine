#pragma once
#include <engine/core/systems_context.h>

struct SystemBase
{
	virtual void execute(SystemsContext& ctx) = 0;
};