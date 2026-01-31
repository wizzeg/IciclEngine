#pragma once

#include <engine/resources/data_structs.h>
#include <engine/game/components.h>
struct AABB
{
	glm::vec3 aabb_min;
	glm::vec3 aabb_max;
};

struct OBB
{
	glm::vec3 obb_center;
	glm::quat obb_rotation;
	glm::vec3 obb_half_extens;
};

struct ContactManifold
{
	entt::entity entity_a = entt::null;
	entt::entity entity_b = entt::null;
	size_t access_order_a;
	size_t access_order_b;
	RigidBodyComponent* rb_a;
	RigidBodyComponent* rb_b;
	glm::vec3 contact_normal = glm::vec3(0.f);
	float penetration_depth = 0.f;
	glm::vec3 contact_points[8];
	//std::vector<glm::vec3> contact_points;
	uint8_t num_contact_points = 0;
	bool has_collision = false;
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

	bool operator==(const BroadPhasePair& other) const {
		return entity_a == other.entity_a && entity_b == other.entity_b;
	}
	bool operator<(const BroadPhasePair& other) const {
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
	bool operator==(const SpatialColliderData& other) const {
		return entity == other.entity;
	}
	bool operator<(const SpatialColliderData& other) const {
		return entity < other.entity;
	}
};

struct CellCoordinates {
	int x, y, z;

	bool operator==(const CellCoordinates& other) const {
		return x == other.x && y == other.y && z == other.z;
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