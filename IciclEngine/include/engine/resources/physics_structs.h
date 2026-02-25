#pragma once

#include <engine/resources/data_structs.h>
#include <engine/game/components.h>
struct alignas(8) AABB
{
	glm::vec3 aabb_min;
	glm::vec3 aabb_max;
};

struct alignas(8) OBB
{
	glm::vec3 obb_center;
	glm::quat obb_rotation;
	glm::vec3 obb_half_extens;
};

struct ContactManifold
{
	entt::entity entity_a = entt::null;
	entt::entity entity_b = entt::null;
	size_t access_order_a = SIZE_MAX;
	size_t access_order_b = SIZE_MAX;
	RigidBodyComponent* rb_a = nullptr;
	RigidBodyComponent* rb_b = nullptr;
	glm::vec3 contact_normal = glm::vec3(0.f);
	float penetration_depth = 0.f;
	glm::vec3 contact_points[8] = { glm::vec3(0),glm::vec3(0),glm::vec3(0),glm::vec3(0),glm::vec3(0),glm::vec3(0),glm::vec3(0),glm::vec3(0) };
	//std::vector<glm::vec3> contact_points;
	uint8_t num_contact_points = 0;
	bool has_collision = false;
	bool landscape_a = false;
	bool landscape_b = false;
	size_t landscape_index = 0;
};

struct PhysicsLayers
{
	uint8_t static_layers = 0b00000000;
	uint8_t dynamic_layers = 0b00000000;
	uint8_t collision_layers = 0b00000000;

	bool const is_static_against(const uint8_t layers) const
	{
		return (static_layers & layers) != 0;
	}
	bool is_dynamic_against(uint8_t layers) const
	{
		return (dynamic_layers & layers) != 0;
	}

	bool is_static_layer(uint8_t layer) const
	{
		if (layer > 7) return false;
		return (static_layers & (1u << layer)) != 0;
	}
	void set_static_layer(uint8_t layer)
	{
		if (layer > 7) return;
		uint8_t mask = (1u << layer);
		dynamic_layers &= ~mask;
		static_layers |= mask;
	}
	void clear_static_layer(uint8_t layer)
	{
		if (layer > 7) return;
		uint8_t mask = (1u << layer);
		static_layers &= ~mask;
	}
	bool is_dynamic_layer(uint8_t layer) const
	{
		if (layer > 7) return false;
		return (dynamic_layers & (1u << layer)) != 0;
	}
	void set_dynamic_layer(uint8_t layer)
	{
		if (layer > 7) return;
		uint8_t mask = (1u << layer);
		static_layers &= ~mask;
		dynamic_layers |= mask;
	}
	void clear_dynamic_layer(uint8_t layer)
	{
		if (layer > 7) return;
		uint8_t mask = (1u << layer);
		dynamic_layers &= ~mask;
	}
	bool const is_collision_against(const PhysicsLayers& other) const
	{
		return (collision_layers & other.collision_layers) != 0;
	}
	bool const is_collision_against(const uint8_t layers) const
	{
		return (collision_layers & layers) != 0;
	}
	bool const is_collision_layer(const uint8_t layer) const
	{
		if (layer > 7) return false;
		return (collision_layers & (1u << layer)) != 0;
	}
	void const set_collision_layer(const uint8_t layer)
	{
		if (layer > 7) return;
		uint8_t mask = (1u << layer);
		collision_layers |= mask;
	}
	void const clear_collision_layer(const uint8_t layer)
	{
		if (layer > 7) return;
		uint8_t mask = (1u << layer);
		collision_layers &= ~mask;
	}

	bool const any_physics_collision(const PhysicsLayers& b) const
	{
		return ( is_dynamic_against(b.dynamic_layers) // is_static_against(b.static_layers) ||
			|| is_static_against(b.dynamic_layers) || is_dynamic_against(b.static_layers));
	}

	bool const operator &(const PhysicsLayers& other) const
	{
		return any_physics_collision(other);
	}
};


struct BroadPhasePair
{
	entt::entity entity_a;
	entt::entity entity_b;
	size_t access_order_a;
	size_t access_order_b;
	RigidBodyComponent* rb_a = nullptr;
	RigidBodyComponent* rb_b = nullptr;
	OBB obb_a;
	OBB obb_b;
	PhysicsLayers layers_a;
	PhysicsLayers layers_b;
	bool landscape_a;
	bool landscape_b;

	bool const operator==(const BroadPhasePair& other) const {
		return entity_a == other.entity_a && entity_b == other.entity_b;
	}
	bool const operator<(const BroadPhasePair& other) const {
		if (entity_a == other.entity_a)
			return entity_b < other.entity_b;
		return entity_a < other.entity_a;
	}
};


struct SpatialColliderData
{
	entt::entity entity;
	size_t access_order;
	RigidBodyComponent* rb = nullptr;
	AABB aabb;
	OBB obb;
	PhysicsLayers layers;
	bool const operator==(const SpatialColliderData& other) const {
		return entity == other.entity;
	}
	bool const operator<(const SpatialColliderData& other) const {
		return entity < other.entity;
	}
};

struct CellCoordinates {
	int x, y, z;

	bool const operator==(const CellCoordinates& other) const {
		return x == other.x && y == other.y && z == other.z;
	}
	bool const operator<(const CellCoordinates& other) const {
		if (x == other.x)
		{
			if (y == other.y)
				return z < other.z;
			return y < other.y;
		}
		return x < other.x;
	}
};

template<>
struct std::hash<CellCoordinates> {
	std::size_t operator()(const CellCoordinates& coord) const {
		std::size_t h1 = std::hash<int>{}(coord.x);
		std::size_t h2 = std::hash<int>{}(coord.y);
		std::size_t h3 = std::hash<int>{}(coord.z);
		return h1 ^ (h2 << 1) ^ (h3 << 2);  // Simple combine
	}
};

struct SpatialColliderPartitioning
{
	std::unordered_map<CellCoordinates, std::vector<SpatialColliderData>> spatial_cells;
	std::vector<BroadPhasePair> broad_phase_data;
};

struct SmallEntry
{
	CellCoordinates coordinates;
	entt::entity entity;
	size_t access_order;
	RigidBodyComponent* rb = nullptr;
	AABB aabb;
	OBB obb;
	PhysicsLayers layers;
	bool asleep = false;
	uint16_t tag;
	bool operator==(const SmallEntry& other) const {
		return coordinates == other.coordinates;
	}
	bool operator<(const SmallEntry& other) const {
		return coordinates < other.coordinates;
	}
};

struct LargeEntry
{
	CellCoordinates coordinates;
	entt::entity entity;
	size_t access_order;
	RigidBodyComponent* rb = nullptr;
	AABB aabb;
	OBB obb;
	PhysicsLayers layers;
	bool operator==(const LargeEntry& other) const {
		return coordinates == other.coordinates;
	}
	bool operator<(const LargeEntry& other) const {
		return coordinates < other.coordinates;
	}
};

struct MassiveEntry
{
	entt::entity entity;
	size_t access_order;
	RigidBodyComponent* rb = nullptr;
	AABB aabb;
	OBB obb;
	PhysicsLayers layers;
	bool asleep = false;
	uint16_t tag;
	bool operator==(const MassiveEntry& other) const {
		return entity == other.entity;
	}
	bool operator<(const MassiveEntry& other) const {
		return entity < other.entity;
	}
};

struct CellEntry
{
	std::vector<SmallEntry> small_entries;
	//std::vector<LargeEntry> large_entries;
	std::vector<MassiveEntry> massive_entries;
};
struct StartStop
{
	size_t start;
	size_t stop;
};

struct PartitionedCellEntry
{
	CellEntry entries;
	std::vector<StartStop> start_stops;
};

struct CollisionInfo
{
	entt::entity entity;
	uint16_t tag;
};

struct CollisionResult
{
	entt::entity self_entity;
	entt::entity other_entity;
	uint32_t self_access_order;
	uint32_t other_access_order;
	uint16_t other_tag;
};