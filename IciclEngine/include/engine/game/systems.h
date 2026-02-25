#pragma once
#include <engine/game/system_base.h>
#include <engine/resources/data_structs.h>
#include <engine/resources/physics_structs.h>
#include <engine/utilities/utilities.h>

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
	float cell_size = 5.f;
	uint8_t max_cells = 9; // if object spans more than 9 cells, register it in seperate container
	size_t chunk_size = 1;
	std::vector<SpatialColliderPartitioning> vectored_spatial_collider_partitioning;
	std::vector<std::unordered_map<CellCoordinates, std::vector<entt::entity>>> vectored_spatial_map;
	std::vector<std::vector<SpatialColliderData>> vectored_collider_data;

	std::vector<CellEntry> vectored_cell_entries;

	std::vector<ContactManifold> landscape_manifolds;
	std::vector<RigidBodyComponent> landscape_rbs;

	HighResolutionTimer timer;

	double timer_spatial = 0;
	double timer_AABB = 0;
	double timer_OBB = 0;
	double timer_manifold = 0;
	double timer_resolve = 0;

	float sleep_linear_threshold = 0.05f;
	float sleep_angular_threshold = 0.05f;

	bool execute(SystemsContext& ctx) override;
	bool overlap(const AABB& a, const AABB& b);
	int divide_int(float input, size_t divisor);
	ContactManifold OBB_collision_test(const BroadPhasePair pair);
	glm::vec3 get_OBB_axis(const OBB& obb, int index) const;
	float project_OBB_on_axis(const OBB& obb, const glm::vec3& axis) const;
	bool test_seperation_axis(const OBB& a, const OBB& b, const glm::vec3& axis, float& min_penetration, glm::vec3& min_axis);

	std::array<glm::vec3, 8> get_OBB_corners(const OBB& obb);
	bool is_point_in_OBB(const glm::vec3& point, const OBB& obb, float tolerance = 0.01f);
	glm::vec3 project_point_to_OBB(const glm::vec3& point, const OBB& obb);
	void generate_contact_points(ContactManifold& manifold, const OBB& obb_a, const OBB& obb_b);

	glm::mat3 calculate_box_inertia_tensor_inverse(float mass, const glm::vec3& half_extents);
	void resolve_collision(ContactManifold& manifold, std::vector<RigidBodyComponent>& landscape_rbs, float dt);

	void generate_landscape_collision();
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

struct HeightMapLoadSystem : SystemBase
{
	bool execute(SystemsContext& ctx) override;
	bool generate_heightmap(const std::string& path, HeightMap& map);
};

struct UIDrawGathering : SystemBase
{
	bool execute(SystemsContext& ctx) override;
	glm::vec2 char_to_offset(const char& c);
};

struct MenuSystem : SystemBase
{
	bool execute(SystemsContext& ctx) override;
	bool menu_open = false;
	bool just_changed = false;
	glm::vec2 mouse_position = glm::vec2(0);
};

struct ReadCollisionResultSystem : SystemBase
{
	bool execute(SystemsContext& ctx) override;
	SystemsStorageObject<std::vector<CollisionResult>>* col_res;
	std::vector<CollisionResult> collisions;
};