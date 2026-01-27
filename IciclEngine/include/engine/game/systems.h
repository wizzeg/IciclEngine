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

struct AABBWriteSpatialPartitionSystem : SystemBase
{
	float cell_size = 5.f;
	std::vector<std::unordered_map<CellCoordinates, std::vector<entt::entity>>> vectored_spatial_map;
	bool execute(SystemsContext& ctx) override;
};

struct AABBReadSpatialPartitionSystem : SystemBase
{
	size_t counter = 0;
	std::vector<std::unordered_map<CellCoordinates, std::vector<entt::entity>>> vectored_spatial_map;
	bool execute(SystemsContext& ctx) override;
};