#pragma once
#include <engine/game/system_base.h>
#include <engine/resources/data_structs.h>
#include <engine/resources/physics_structs.h>

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

struct PhysicsSystem : SystemBase
{
	float cell_size = 10.f;
	size_t chunk_size = 1;
	std::vector<SpatialColliderPartitioning> vectored_spatial_collider_partitioning;
	std::vector<std::unordered_map<CellCoordinates, std::vector<entt::entity>>> vectored_spatial_map;
	std::vector<std::vector<SpatialColliderData>> vectored_collider_data;
	bool execute(SystemsContext& ctx) override;
	bool overlap(const AABB& a, const AABB& b);
	int divide_int(int input, size_t divisor);
	ContactManifold OBB_collision_test(const BroadPhasePair pair);
	glm::vec3 get_OBB_axis(const OBB& obb, int index) const;
	float project_OBB_on_axis(const OBB& obb, const glm::vec3& axis) const;
	bool test_seperation_axis(const OBB& a, const OBB& b, const glm::vec3& axis, float& min_penetration, glm::vec3& min_axis);

	std::array<glm::vec3, 8> get_OBB_corners(const OBB& obb);
	bool is_point_in_OBB(const glm::vec3& point, const OBB& obb, float tolerance = 0.01f);
	glm::vec3 project_point_to_OBB(const glm::vec3& point, const OBB& obb);
	void generate_contact_points(ContactManifold& manifold, const OBB& obb_a, const OBB& obb_b);

	glm::mat3 calculate_box_inertia_tensor_inverse(float mass, const glm::vec3& half_extents);
	void resolve_collision(ContactManifold& manifold, float dt);

	size_t count = 0;
};

struct AABBReadSpatialPartitionSystem : SystemBase
{
	size_t counter = 0;
	std::vector<std::unordered_map<CellCoordinates, std::vector<entt::entity>>> vectored_spatial_map;
	bool execute(SystemsContext& ctx) override;
	
};

struct BoardPhaseCollision : SystemBase
{
	bool execute(SystemsContext& ctx) override;
};

struct SpawnCubeSystem : SystemBase
{
	bool execute(SystemsContext& ctx) override;
	bool has_spawned = false;
};

struct DestroyCubeSystem : SystemBase
{
	bool execute(SystemsContext& ctx) override;
	bool has_destroyed = false;
};

struct ECBCreatorSystem : SystemBase
{
	bool execute(SystemsContext& ctx) override;
};

struct SyncSystem :SystemBase
{
	bool execute(SystemsContext& ctx) override { ctx.gen_sync(); ctx.entt_sync(); return false; }
};


