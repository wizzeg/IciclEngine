#include <engine/game/systems.h>
#include <engine/game/components.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>
#include <engine/utilities/macros.h>
#include <engine/resources/data_structs.h>
#include <stb_image/stb_image.h>


bool MoveSystem::execute(SystemsContext& ctx)
{
	
	time += ctx.get_delta_time();
	//ctx.each_entity<TransformDynamicComponent, SpawnPositionComponent>(
	//	[&](entt::entity entity, TransformDynamicComponent& transform, SpawnPositionComponent& spawn)
	//	{
	//		float amplitude = 5.f;
	//		float frequency = 5.f;
	//		float offset = amplitude * glm::sin(frequency * transform.position.x + time);
	//		transform.position.y = spawn.spawn_position.y + offset;
	//	});
	float current_time = time;
	//ctx.enqueue_parallel_each(
	//	WithRead<SpawnPositionComponent>{},
	//	WithWrite<TransformDynamicComponent>{},
	//	[current_time](entt::entity entity, SpawnPositionComponent& spawn, TransformDynamicComponent& transform)
	//	{
	//		float amplitude = 5.f;
	//		float frequency = 5.f;
	//		float offset = amplitude * glm::sin(frequency * transform.position.x + current_time);
	//		transform.position.y = spawn.spawn_position.y + offset;
	//	}, 256);
	//ctx.enqueue_parallel_each(
	//	Group<TransformDynamicComponent, SpawnPositionComponent>{},
	//	[time = this->time](entt::entity entity, TransformDynamicComponent& transform, SpawnPositionComponent& spawn)
	//	{
	//		float amplitude = 5.f;
	//		float frequency = 5.f;
	//		float offset = amplitude * glm::sin(frequency * transform.position.x + time);
	//		transform.position.y = spawn.spawn_position.y + offset;
	//	}, 256);

	//ctx.enqueue(
	//	[&ctx, current_time]() {
			return ctx.enqueue_parallel_each(
				WithRead<const SpawnPositionComponent, const RenderComponent>{},
				WithWrite<TransformDynamicComponent>{},
				[current_time](const entt::entity entity, const SpawnPositionComponent& spawn, const RenderComponent& render, TransformDynamicComponent& transform)
				{
					float amplitude = 5.f;
					float frequency = 5.f;
					float offset = amplitude * glm::sin(frequency * transform.position.x + current_time);
					transform.position.y = spawn.spawn_position.y + offset;
				}, 512);
		//});
}

bool TransformCalculationSystem::execute(SystemsContext& ctx)
{
	//ctx.enqueue_parallel_each(
	//	WithWrite<TransformDynamicComponent>{},
	//	Without<CameraComponent>{},
	//	[](entt::entity entity, TransformDynamicComponent& world_pos)
	//	{
	//		if (world_pos.overide_quaternion)
	//		{
	//			world_pos.rotation_euler_do_not_use.x = std::fmod(world_pos.rotation_euler_do_not_use.x, 360.0f);
	//			if (world_pos.rotation_euler_do_not_use.x < 0.0f) world_pos.rotation_euler_do_not_use.x += 360.0f;
	//			world_pos.rotation_euler_do_not_use.y = std::fmod(world_pos.rotation_euler_do_not_use.y, 360.0f);
	//			if (world_pos.rotation_euler_do_not_use.y < 0.0f) world_pos.rotation_euler_do_not_use.y += 360.0f;
	//			world_pos.rotation_euler_do_not_use.z = std::fmod(world_pos.rotation_euler_do_not_use.z, 360.0f);
	//			if (world_pos.rotation_euler_do_not_use.z < 0.0f) world_pos.rotation_euler_do_not_use.z += 360.0f;

	//			world_pos.rotation_quat = glm::quat(glm::radians(world_pos.rotation_euler_do_not_use));
	//		}
	//		if (world_pos.get_euler_angles) /// 0.5ms
	//		{
	//			world_pos.rotation_euler_do_not_use = glm::degrees(glm::eulerAngles(world_pos.rotation_quat));
	//			//if (world_pos.overide_quaternion)
	//			//	world_pos.get_euler_angles = false;
	//		}

	//		world_pos.model_matrix = glm::translate(glm::mat4(1.0f), world_pos.position); // 0.47ms
	//		world_pos.model_matrix *= glm::mat4_cast(world_pos.rotation_quat); // 1.47ms
	//		world_pos.model_matrix = glm::scale(world_pos.model_matrix, world_pos.scale); // 0.4ms
	//	},256);
	//ctx.sync();

	//ctx.enqueue(
	//	[&ctx]() {
			return ctx.enqueue_parallel_each(
				WithWrite<TransformDynamicComponent>{},
				Without<CameraComponent>{},
				[](const entt::entity entity, TransformDynamicComponent& world_pos)
				{
					if (world_pos.overide_quaternion)
					{
						world_pos.rotation_euler_do_not_use.x = std::fmod(world_pos.rotation_euler_do_not_use.x, 360.0f);
						if (world_pos.rotation_euler_do_not_use.x < 0.0f) world_pos.rotation_euler_do_not_use.x += 360.0f;
						world_pos.rotation_euler_do_not_use.y = std::fmod(world_pos.rotation_euler_do_not_use.y, 360.0f);
						if (world_pos.rotation_euler_do_not_use.y < 0.0f) world_pos.rotation_euler_do_not_use.y += 360.0f;
						world_pos.rotation_euler_do_not_use.z = std::fmod(world_pos.rotation_euler_do_not_use.z, 360.0f);
						if (world_pos.rotation_euler_do_not_use.z < 0.0f) world_pos.rotation_euler_do_not_use.z += 360.0f;

						world_pos.rotation_quat = glm::quat(glm::radians(world_pos.rotation_euler_do_not_use));
					}
					if (world_pos.get_euler_angles) /// 0.5ms
					{
						world_pos.rotation_euler_do_not_use = glm::degrees(glm::eulerAngles(world_pos.rotation_quat));
						//if (world_pos.overide_quaternion)
						//	world_pos.get_euler_angles = false;
					}

					world_pos.model_matrix = glm::translate(glm::mat4(1.0f), world_pos.position); // 0.47ms
					world_pos.model_matrix *= glm::mat4_cast(world_pos.rotation_quat); // 1.47ms
					world_pos.model_matrix = glm::scale(world_pos.model_matrix, world_pos.scale); // 0.4ms
				}, 512);
			
		//});
	//ctx.enqueue_each(
	//	WithRead<TransformDynamicComponent>{},
	//	[](entt::entity, TransformDynamicComponent& transform)
	//	{

	//	}
	//);
}

bool PhysicsSystem::execute(SystemsContext& ctx)
{
	size_t num_threads = ctx.num_threads();

	// consider just storing pointer to this in the storage
	vectored_spatial_collider_partitioning.clear();
	vectored_spatial_collider_partitioning.resize(num_threads);

	//vectored_collider_data.clear();
	//vectored_spatial_map.clear();
	//vectored_collider_data.resize(num_threads);
	//vectored_spatial_map.resize(num_threads);
	count++;
	if (count > 100)
	{
		PRINTLN("launching threads");

	}
	ctx.enqueue_parallel_data_each(
		WithRead<TransformDynamicComponent, BoundingBoxComponent>{},
		[this, &ctx](SpatialColliderPartitioning& spatial_partition,
			const entt::entity entity,
			const TransformDynamicComponent& transform,
			const BoundingBoxComponent& bbox, // need rigid body? No not really
			size_t visit_order)
		{
			auto& cells = spatial_partition.spatial_cells;

			const glm::mat4& model_matrix = transform.model_matrix;

			
			glm::mat3 scaled_rotation = glm::mat3(model_matrix); // I should not use model matrix, should use quat I think
			glm::mat3 rotation; // need to remove scaling from model matrix
			rotation[0] = glm::normalize(model_matrix[0]);
			rotation[1] = glm::normalize(model_matrix[1]);  
			rotation[2] = glm::normalize(model_matrix[2]);
			glm::vec3 scale;
			scale.x = glm::length(model_matrix[0]);
			scale.y = glm::length(model_matrix[1]);  
			scale.z = glm::length(model_matrix[2]);
			glm::vec3 half_extents = bbox.box_extents * 0.5f * scale;
			glm::quat obb_rotation = glm::quat_cast(rotation);
			glm::vec3 offset_rot_scaled_box = glm::vec3(model_matrix * glm::vec4(bbox.offset, 1.f));

			glm::mat3 max_scaled_rotation = glm::mat3(
				glm::abs(scaled_rotation[0]),
				glm::abs(scaled_rotation[1]),
				glm::abs(scaled_rotation[2])
			);

			glm::vec3 world_half_extents = max_scaled_rotation * half_extents;

			glm::vec3 aabb_min = offset_rot_scaled_box - world_half_extents;
			glm::vec3 aabb_max = offset_rot_scaled_box + world_half_extents;
			AABB aabb{ aabb_min, aabb_max };
			OBB obb{ offset_rot_scaled_box, obb_rotation, half_extents };

			CellCoordinates min_cell = {
			static_cast<int>(std::floor(aabb_min.x / cell_size)),
			static_cast<int>(std::floor(aabb_min.y / cell_size)),
			static_cast<int>(std::floor(aabb_min.z / cell_size))
			};

			CellCoordinates max_cell = {
				static_cast<int>(std::floor(aabb_max.x / cell_size)),
				static_cast<int>(std::floor(aabb_max.y / cell_size)),
				static_cast<int>(std::floor(aabb_max.z / cell_size))
			};

			if (auto rb = ctx.try_get<RigidBodyComponent>(entity))
			{
				if (rb->recalculate_inverse_inertia_tensor)
				{
					rb->recalculate_inverse_inertia_tensor = false;
					rb->inverse_inertia_tensor_local = calculate_box_inertia_tensor_inverse(rb->mass, half_extents);
				}
			}

			for (int x = min_cell.x; x <= max_cell.x; ++x)
				for (int y = min_cell.y; y <= max_cell.y; ++y)
					for (int z = min_cell.z; z <= max_cell.z; ++z)
						cells[{x, y, z}].emplace_back(entity, visit_order, ctx.try_get<RigidBodyComponent>(entity), aabb, obb);// need to filter out none-rigid bodies
			

		}, vectored_spatial_collider_partitioning, num_threads, true);
	// also do one for all landscapes...
	
	ctx.entt_sync(); // could push vectored_spatial_map into the storage to retrrieve this->vectored_spatial_map later to combine

	// merge spatial data first.
	SpatialColliderPartitioning merged_partition;

	for (auto& thread_partition : vectored_spatial_collider_partitioning)
	{
		for (auto& [cell_coord, colliders] 
			: thread_partition.spatial_cells) 
		{
			auto& merged_cell = merged_partition.spatial_cells[cell_coord];
			merged_cell.insert(merged_cell.end(), colliders.begin(), colliders.end());
		}
	}
	// then split it into chunks
	// so this was a bad idea, I need to fix this later, to avoid this whole thing.
	std::unordered_map<CellCoordinates, std::vector<CellCoordinates>> chunks;

	for (auto& [cell_coord, _] : merged_partition.spatial_cells)
	{
		CellCoordinates cunk
		{
			divide_int(cell_coord.x, chunk_size),
			divide_int(cell_coord.y, chunk_size),
			divide_int(cell_coord.z, chunk_size)
		};
		chunks[cunk].push_back(cell_coord);
	}
	std::vector<std::pair<CellCoordinates, std::vector<CellCoordinates>>> vec_chunks;
	vec_chunks.reserve(chunks.size());
	// split up the chunks...
	for (auto& [chunk, coords] : chunks)
	{
		vec_chunks.emplace_back(chunk, std::move(coords));
	}
	std::vector<std::vector<BroadPhasePair>> thread_pairs(num_threads);
	const size_t chunks_per_thread = (vec_chunks.size() + num_threads - 1) / num_threads;
	// would be nice to note start unecessary threads..

	// how check AABBs and generate pairs...
	for (size_t i = 0; i < num_threads; i++)
	{
		
		size_t thread_id = i;
		ctx.enqueue(
			[this, &vec_chunks, &merged_partition, &thread_pairs, chunks_per_thread, thread_id]()
			{
				size_t start_chunk = chunks_per_thread * thread_id;
				size_t end_chunk = std::min(start_chunk + chunks_per_thread, vec_chunks.size());
				auto& pairs = thread_pairs[thread_id];
				pairs.reserve(500);
				for (size_t chunk = start_chunk; chunk < end_chunk; ++chunk)
				{
					
					const auto& [chunk_coord, cell_coords] = vec_chunks[chunk];
					for (const auto& cell_coord : cell_coords)
					{
						auto& colliders = merged_partition.spatial_cells[cell_coord];
						for (size_t i = 0; i < colliders.size(); i++)
						{
							for (size_t j = i + 1; j < colliders.size(); j++)
							{
								// here we'de needto check "colliders" what they are.
								if (colliders[i].entity != colliders[j].entity && overlap(colliders[i].aabb, colliders[j].aabb)) // check if potential collision
								{
									// I think here is where we generate new info for the landscape... call it entt::null
									// we'll need to create a new obb... but we need more information to make these decisions
									if ((uint32_t)colliders[i].entity < (uint32_t)colliders[j].entity)
									{
										pairs.emplace_back(colliders[i].entity, colliders[j].entity,
											colliders[i].access_order, colliders[j].access_order,
											colliders[i].rb, colliders[j].rb,
											colliders[i].obb, colliders[j].obb);
									}
									else
									{
										pairs.emplace_back(colliders[j].entity, colliders[i].entity,
											colliders[j].access_order, colliders[i].access_order,
											colliders[i].rb, colliders[j].rb,
											colliders[j].obb, colliders[i].obb);
									}
								}
							}
						}
					}
				}

				//if (count > 100)
				//{
				//	PRINTLN("thread doing work");
				//}

				// sort
				std::sort(pairs.begin(), pairs.end());
				// purge
				pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
				// replace
				//spatial_collider_partitioning.broad_phase_data = pairs;
				if (pairs.size() > 0 && count > 100)
				{
					PRINTLN("thread({}) - potential collisions: {}", thread_id, pairs.size());
				}
				// can clear old data now.
			});
	}

	ctx.gen_sync();
	// merge pairs
	std::vector<BroadPhasePair> merged_pairs;
	for (size_t i = 0; i < num_threads; i++) {
		merged_pairs.insert(merged_pairs.end(), std::make_move_iterator(thread_pairs[i].begin()),
			std::make_move_iterator(thread_pairs[i].end()));
	}
	// prep for narrow phase?
	std::sort(merged_pairs.begin(), merged_pairs.end());
	merged_pairs.erase(std::unique(merged_pairs.begin(), merged_pairs.end()), merged_pairs.end());
	
	// now I'm not sure what to do?
	// split into parallel jobs to calculate all contact points? Probably right?
	const size_t pairs_per_thread = (merged_pairs.size() + num_threads - 1) / num_threads;
	std::vector<std::vector<ContactManifold>> vec_manifolds(num_threads);
	for (size_t i = 0; i < num_threads; i++)
	{
		size_t thread_id = i;

		auto& manifolds = vec_manifolds[thread_id];
		ctx.enqueue(
			[this, thread_id, pairs_per_thread, &manifolds, &merged_pairs]() 
			{
				
				size_t start_pair = pairs_per_thread * thread_id;
				size_t end_pair = std::min(start_pair + pairs_per_thread, merged_pairs.size());
				//manifolds.reserve(end_pair - start_pair);
				for (size_t i = start_pair; i < end_pair; ++i)
				{
					if (merged_pairs[i].rb_a && merged_pairs[i].rb_b)
					{
						ContactManifold manfild = OBB_collision_test(merged_pairs[i]); // need to check if it has RB or not
						if (manfild.has_collision)
						{
							manifolds.push_back(manfild);
						}
					}
					else
					{
						// both don't have rigidbody, so store collision information only
						// actually should probably store this info somewhere in both cases
						//store information for collision info stuff
						// but where? I'm not sure...
					}

				}
			});
	}
	ctx.gen_sync();

	std::vector<ContactManifold> merged_manifolds;
	for (auto& manifold : vec_manifolds)
	{
		merged_manifolds.insert(merged_manifolds.end(),
			std::make_move_iterator(manifold.begin()),
			std::make_move_iterator(manifold.end()));
	}

	// now time for physics solver ... but I should have gathered rigidbodies by now... hm.. done that now
	// but before this, I should not have put those that lack rb into the manifolds.
	std::sort(merged_manifolds.begin(), merged_manifolds.begin(), []
	(const ContactManifold& man_a, const ContactManifold& man_b)
		{
			if (man_a.access_order_a == man_b.access_order_b)
				return man_a.access_order_b < man_b.access_order_b;
			return man_a.access_order_a < man_b.access_order_b;
		});

	// now I need to "just" do physics resolve
	const int solver_iterations = 5; // 5-10 typical

	for (int iteration = 0; iteration < solver_iterations; iteration++)
	{
		for (auto& manifold : merged_manifolds)
		{
			if (true)// wait what was I doing here? 
			{

			}
			resolve_collision(manifold, ctx.get_delta_time());
		}
	}

	// now realize the rb changes into position and rotation
	float dt = (float)ctx.get_delta_time();
	ctx.enqueue_parallel_each(WithRead<>{},
		WithWrite<RigidBodyComponent, TransformDynamicComponent>{},
		[this, dt](const entt::entity entity, RigidBodyComponent& rb, TransformDynamicComponent& transform)
		{
			// --------------------------------------
			// Integrate Linear Velocity → Position
			// --------------------------------------
			rb.position += rb.linear_velocity * dt;

			// --------------------------------------
			// Integrate Angular Velocity → Orientation
			// --------------------------------------
			glm::quat omega_quat(0.0f, rb.angular_velocity.x, rb.angular_velocity.y, rb.angular_velocity.z);
			glm::quat delta_q = 0.5f * omega_quat * rb.rotation;
			rb.rotation += delta_q * dt;

			// CRITICAL: Normalize quaternion to prevent drift
			rb.rotation = glm::normalize(rb.rotation);

			// --------------------------------------
			// Optional: Apply Damping
			// --------------------------------------
			rb.linear_velocity *= 1.0f;
			rb.angular_velocity *= 1.0f;

			// --------------------------------------
			// Update Transform for Rendering
			// --------------------------------------
			// Extract scale from current transform
			glm::vec3 scale(
				glm::length(transform.model_matrix[0]),
				glm::length(transform.model_matrix[1]),
				glm::length(transform.model_matrix[2])
			);
			transform.position = rb.position;
			transform.rotation_quat = rb.rotation;
			// Rebuild model matrix: Translation * Rotation * Scale
			transform.model_matrix =
				glm::translate(glm::mat4(1.0f), rb.position) *
				glm::mat4_cast(rb.rotation) *
				glm::scale(glm::mat4(1.0f), scale);
		}, 512);

	if (count > 100)
	{
		PRINTLN("detected collisions {}", merged_manifolds.size());
		count = 0;
	}


	// I could send in an extra lambda in the above, as an "after completion" to insert in parallel in chunks
	return false;
}

bool PhysicsSystem::overlap(const AABB& a, const AABB& b)
{
	return !(a.aabb_max.x < b.aabb_min.x || a.aabb_min.x > b.aabb_max.x ||
		a.aabb_max.y < b.aabb_min.y || a.aabb_min.y > b.aabb_max.y ||
		a.aabb_max.z < b.aabb_min.z || a.aabb_min.z > b.aabb_max.z);
}

int PhysicsSystem::divide_int(int input, size_t divisor)
{
	if (input >= 0)
	{
		return input / (int)divisor;
	}
	else
	{
		return -((-input) / (int)divisor);
	}
}

ContactManifold PhysicsSystem::OBB_collision_test(const BroadPhasePair pair)
{
	ContactManifold manifold;
	manifold.entity_a = pair.entity_a;
	manifold.entity_b = pair.entity_b;
	manifold.has_collision = false;
	manifold.penetration_depth = FLT_MAX;

	glm::vec3 min_axis(0.0f);
	float min_penetration = FLT_MAX;

	// Test face normals of A (3 axes)
	for (int i = 0; i < 3; i++)
	{
		glm::vec3 axis = get_OBB_axis(pair.obb_a, i);
		if (!test_seperation_axis(pair.obb_a, pair.obb_b, axis, min_penetration, min_axis))
			return manifold;
	}

	// Test face normals of B (3 axes)
	for (int i = 0; i < 3; i++)
	{
		glm::vec3 axis = get_OBB_axis(pair.obb_b, i);
		if (!test_seperation_axis(pair.obb_a, pair.obb_b, axis, min_penetration, min_axis))
			return manifold;
	}

	// Test edge cross products (9 axes)
	for (int i = 0; i < 3; i++)
	{
		glm::vec3 axisA = get_OBB_axis(pair.obb_a, i);
		for (int j = 0; j < 3; j++)
		{
			glm::vec3 axisB = get_OBB_axis(pair.obb_b, j);
			glm::vec3 axis = glm::cross(axisA, axisB);

			if (!test_seperation_axis(pair.obb_a, pair.obb_b, axis, min_penetration, min_axis))
				return manifold;
		}
	}

	// Collision detected
	manifold.has_collision = true;
	manifold.contact_normal = min_axis;
	manifold.penetration_depth = min_penetration;

	// Generate contact points (simplified - single point at center)
	// For full implementation, you'd need proper contact point generation
	//glm::vec3 contactPoint = (pair.obb_a.obb_center + pair.obb_b.obb_center) * 0.5f;
	//manifold.contact_points.push_back(contactPoint);
	generate_contact_points(manifold, pair.obb_a, pair.obb_b);
	manifold.access_order_a = pair.access_order_a;
	manifold.access_order_b = pair.access_order_b;
	manifold.rb_a = pair.rb_a;
	manifold.rb_b = pair.rb_b;
	return manifold;
}

glm::vec3 PhysicsSystem::get_OBB_axis(const OBB& obb, int index) const
{
	glm::mat3 rotatoion = glm::mat3_cast(obb.obb_rotation);
	return rotatoion[index];
}

float PhysicsSystem::project_OBB_on_axis(const OBB& obb, const glm::vec3& axis) const
{
	glm::mat3 rotation = glm::mat3_cast(obb.obb_rotation);
	float projection = 0.0f;
	for (int i = 0; i < 3; i++)
		projection += glm::abs(glm::dot(axis, rotation[i])) * obb.obb_half_extens[i];
	return projection;
}

bool PhysicsSystem::test_seperation_axis(const OBB& a, const OBB& b, const glm::vec3& axis, float& min_penetration, glm::vec3& min_axis)
{
	float axisLengthSquared = glm::dot(axis, axis);

	// Skip near-zero axes (parallel edges)
	if (axisLengthSquared < 1e-6f)
		return true;

	glm::vec3 normalizedAxis = axis / glm::sqrt(axisLengthSquared);

	float aProjection = project_OBB_on_axis(a, normalizedAxis);
	float bProjection = project_OBB_on_axis(b, normalizedAxis);

	glm::vec3 offset = b.obb_center - a.obb_center;
	float distance = glm::abs(glm::dot(offset, normalizedAxis));

	float overlap = aProjection + bProjection - distance;

	if (overlap < 0.0f)
		return false; // Separating axis found

	// Track minimum penetration
	if (overlap < min_penetration)
	{
		min_penetration = overlap;
		min_axis = normalizedAxis;

		// Ensure normal points from A to B
		if (glm::dot(offset, normalizedAxis) < 0.0f)
			min_axis = -min_axis;
	}

	return true;
}

std::array<glm::vec3, 8> PhysicsSystem::get_OBB_corners(const OBB& obb)
{
	// Get the 3 axis directions of the box in world space
	glm::mat3 rot = glm::mat3_cast(obb.obb_rotation);
	glm::vec3 axes[3] = { rot[0], rot[1], rot[2] };
	// axes[0] = X axis of the box
	// axes[1] = Y axis of the box  
	// axes[2] = Z axis of the box

	std::array<glm::vec3, 8> corners;

	// Generate all 8 combinations of +/- half extents
	for (int i = 0; i < 8; i++)
	{
		glm::vec3 corner = obb.obb_center; // Start at center

		// i = 0: (-, -, -)  →  i = 1: (+, -, -)
		// i = 2: (-, +, -)  →  i = 3: (+, +, -)
		// ... and so on for all 8 corners

		// Bit 0 controls X: 0 = negative, 1 = positive
		corner += axes[0] * obb.obb_half_extens.x * ((i & 1) ? 1.0f : -1.0f);

		// Bit 1 controls Y
		corner += axes[1] * obb.obb_half_extens.y * ((i & 2) ? 1.0f : -1.0f);

		// Bit 2 controls Z
		corner += axes[2] * obb.obb_half_extens.z * ((i & 4) ? 1.0f : -1.0f);

		corners[i] = corner;
	}
	return corners;
}

bool PhysicsSystem::is_point_in_OBB(const glm::vec3& point, const OBB& obb, float tolerance)
{
	glm::vec3 local = point - obb.obb_center;
	glm::mat3 rot = glm::mat3_cast(obb.obb_rotation);

	for (int i = 0; i < 3; i++)
	{
		float distance = glm::abs(glm::dot(local, rot[i]));
		if (distance > obb.obb_half_extens[i] + tolerance)
			return false;
	}
	return true;
}

glm::vec3 PhysicsSystem::project_point_to_OBB(const glm::vec3& point, const OBB& obb)
{
	glm::vec3 local = point - obb.obb_center;
	glm::mat3 rot = glm::mat3_cast(obb.obb_rotation);

	glm::vec3 result = obb.obb_center;
	for (int i = 0; i < 3; i++)
	{
		float distance = glm::dot(local, rot[i]);
		distance = glm::clamp(distance, -obb.obb_half_extens[i], obb.obb_half_extens[i]);
		result += rot[i] * distance;
	}
	return result;
}

void PhysicsSystem::generate_contact_points(ContactManifold& manifold, const OBB& obb_a, const OBB& obb_b)
{
	//manifold.contact_points.clear();

	const float epsilon = 0.05f; // Tolerance for "on surface"

	// Method 1: Check corners of A against B
	auto corners_a = get_OBB_corners(obb_a);
	for (const auto& corner : corners_a)
	{
		if (is_point_in_OBB(corner, obb_b, epsilon))
		{
			if (manifold.num_contact_points >= 8)
			{
				break;
			}
			// Project onto B's surface along contact normal
			glm::vec3 contact = corner - manifold.contact_normal * (manifold.penetration_depth * 0.5f);
			//manifold.contact_points.push_back(contact);
			manifold.contact_points[manifold.num_contact_points++] = contact;

		}
	}

	// Method 2: Check corners of B against A
	auto corners_b = get_OBB_corners(obb_b);
	for (const auto& corner : corners_b)
	{
		if (is_point_in_OBB(corner, obb_a, epsilon))
		{
			if (manifold.num_contact_points >= 8)
			{
				break;
			}
			// Project onto A's surface along contact normal
			glm::vec3 contact = corner + manifold.contact_normal * (manifold.penetration_depth * 0.5f);
			// manifold.contact_points.push_back(contact);
			manifold.contact_points[manifold.num_contact_points++] = contact;

		}
	}

	// Remove duplicate/very close contact points
	for (size_t i = 0; i < manifold.num_contact_points; i++) //manifold.contact_points.size()
	{
		for (size_t j = i + 1; j < manifold.num_contact_points; )
		{
			float dist = glm::length(manifold.contact_points[i] - manifold.contact_points[j]);
			if (dist < 0.01f) // Merge close points
			{
				//manifold.contact_points.erase(manifold.contact_points.begin() + j);
				for (size_t k = j; k < manifold.num_contact_points; k--)
				{
					if (k+1 < manifold.num_contact_points)
					{
						manifold.contact_points[k] = manifold.contact_points[k +1];
					}
				}
				--manifold.num_contact_points;
			}
			else
			{
				j++;
			}
		}
	}

	// Fallback: if no points found, use center approximation
	if (manifold.num_contact_points < 1)//manifold.contact_points.empty()
	{
		glm::vec3 contact = (obb_a.obb_center + obb_b.obb_center) * 0.5f;
		//manifold.contact_points.push_back(contactPoint);
		manifold.contact_points[manifold.num_contact_points++] = contact;
	}

	// Limit to 4 best points (optional, for stability)
	if (manifold.num_contact_points > 4)
	{
		// Keep the 4 points that form the largest area
		// (This is complex - for now just keep first 4)
		//manifold.contact_points.resize(4);
		manifold.num_contact_points = 4;
	}
}

glm::mat3 PhysicsSystem::calculate_box_inertia_tensor_inverse(float mass, const glm::vec3& half_extents)
{
	// For a box, the inertia tensor is diagonal (no off-diagonal terms)
	// Formula: I_xx = (m/12) * (h_y² + h_z²), etc.
	// We want the INVERSE, so: I_xx^-1 = 12 / (m * (h_y² + h_z²))

	if (mass == 0.0f) // Infinite mass (immovable)
	{
		return glm::mat3(0.0f); // Zero inverse = infinite inertia
	}

	float hx2 = half_extents.x * half_extents.x;
	float hy2 = half_extents.y * half_extents.y;
	float hz2 = half_extents.z * half_extents.z;

	// Diagonal matrix (only x, y, z components, no cross terms)
	return glm::mat3(
		12.0f / (mass * (hy2 + hz2)), 0, 0,  // X rotation resistance
		0, 12.0f / (mass * (hx2 + hz2)), 0,  // Y rotation resistance
		0, 0, 12.0f / (mass * (hx2 + hy2))   // Z rotation resistance
	);
}

void PhysicsSystem::resolve_collision(ContactManifold& manifold, float dt)
{
	// ==========================================
 // SAFETY CHECK
 // ==========================================
	RigidBodyComponent* rb_a = manifold.rb_a;
	RigidBodyComponent* rb_b = manifold.rb_b;
	if (!rb_a || !rb_b) return;

	// ==========================================
	// MATERIAL PROPERTIES
	// ==========================================
	// You can get these from the rigid bodies or use constants
	const float restitution = (manifold.rb_a->restitution + manifold.rb_b->restitution) * 0.5f;//0.7f;  // Bounciness
	const float friction = (manifold.rb_a->friction + manifold.rb_b->friction) * 0.5f;//0.5f;     // Surface friction

	// ==========================================
	// STEP 1: POSITION CORRECTION (Baumgarte Stabilization)
	// ==========================================
	// Problem: Due to floating point errors and discrete timesteps,
	// objects can penetrate slightly into each other.
	// Solution: Push them apart gradually over several frames.

	const float penetration_slop = 0.01f;    // Allow 1cm of overlap (prevents jitter)
	const float baumgarte_factor = 0.2f;     // Correct 20% of penetration per iteration

	if (manifold.penetration_depth > penetration_slop)
	{
		// How much penetration do we need to fix?
		float excess_penetration = manifold.penetration_depth - penetration_slop;

		// Only correct a fraction of it (stability)
		float correction_amount = excess_penetration * baumgarte_factor;

		// Total inverse mass (if both movable, they share the work)
		float total_inv_mass = rb_a->inverse_mass + rb_b->inverse_mass;

		if (total_inv_mass > 0.0f) // At least one object is movable
		{
			// The correction vector (how much to move apart)
			glm::vec3 correction = manifold.contact_normal * correction_amount;

			// Move each object proportional to its inverse mass
			// Lighter object (higher inverse_mass) moves MORE
			// Heavier object (lower inverse_mass) moves LESS
			// Immovable object (inverse_mass = 0) doesn't move at all
			rb_a->position -= correction * (rb_a->inverse_mass / total_inv_mass);
			rb_b->position += correction * (rb_b->inverse_mass / total_inv_mass);
		}
	}

	// ==========================================
	// STEP 2: VELOCITY RESOLUTION (Impulse-Based)
	// ==========================================
	// Now we need to change the velocities to simulate bounce and friction.
	// We do this for EACH contact point separately.

	for (size_t i = 0; i < manifold.num_contact_points; i++)
	{
		const glm::vec3& contact_point = manifold.contact_points[i];
		
		// --------------------------------------
		// 2.1: Calculate Moment Arms
		// --------------------------------------
		// The "moment arm" is the vector from center of mass to contact point.
		// This determines the TORQUE (rotational force) that will be applied.
		// 
		// Think of opening a door:
		// - Push near the hinge (small r) → hard to open (small torque)
		// - Push at the handle (large r) → easy to open (large torque)
		glm::vec3 r_a = contact_point - rb_a->position;
		glm::vec3 r_b = contact_point - rb_b->position;

		// --------------------------------------
		// 2.2: Calculate Velocity at Contact Point
		// --------------------------------------
		// A point on a rotating object has velocity from TWO sources:
		// 1. Linear velocity (the whole object moving)
		// 2. Rotational velocity (the object spinning)
		//
		// Formula: v_point = v_linear + ω × r
		// Where ω (omega) is angular velocity and × is cross product
		//
		// The cross product ω × r gives the linear velocity caused by rotation.
		// Direction: perpendicular to both ω and r (right-hand rule)
		// Magnitude: |ω| * |r| * sin(angle)
		glm::vec3 velocity_a = rb_a->linear_velocity + glm::cross(rb_a->angular_velocity, r_a);
		glm::vec3 velocity_b = rb_b->linear_velocity + glm::cross(rb_b->angular_velocity, r_b);

		// How fast are the objects moving TOWARDS each other?
		glm::vec3 relative_velocity = velocity_b - velocity_a;

		// Component along the contact normal (positive = separating, negative = approaching)
		float velocity_along_normal = glm::dot(relative_velocity, manifold.contact_normal);

		// If objects are already separating, don't apply impulse
		if (velocity_along_normal > 0.0f)
			continue;

		// --------------------------------------
		// 2.3: Transform Inertia Tensor to World Space
		// --------------------------------------
		// The inertia tensor is stored in LOCAL space (object's own coordinates).
		// We need it in WORLD space to calculate torques correctly.
		//
		// Transformation formula: I_world = R * I_local * R^T
		// Where R is the rotation matrix and R^T is its transpose
		//
		// Why? The inertia tensor describes how mass is distributed.
		// When the object rotates, this distribution rotates with it.
		glm::mat3 rotation_matrix_a = glm::mat3_cast(rb_a->rotation);
		glm::mat3 rotation_matrix_b = glm::mat3_cast(rb_b->rotation);

		// Transform: Rotate, apply local inertia, rotate back
		glm::mat3 inv_inertia_world_a =
			rotation_matrix_a *
			rb_a->inverse_inertia_tensor_local *
			glm::transpose(rotation_matrix_a);

		glm::mat3 inv_inertia_world_b =
			rotation_matrix_b *
			rb_b->inverse_inertia_tensor_local *
			glm::transpose(rotation_matrix_b);

		// --------------------------------------
		// 2.4: Calculate Normal Impulse Magnitude
		// --------------------------------------
		// This is the core physics calculation!
		// We need to find how much force (impulse) to apply.
		//
		// The formula comes from solving the constraint:
		// "After the impulse, the relative velocity along the normal should reverse"
		//
		// Derivation (simplified):
		// v_new = v_old + impulse/mass
		// We want: v_new = -e * v_old  (where e = restitution)
		// Solving: impulse = -(1 + e) * v_old * effective_mass

		// First, calculate how the impulse affects angular velocity
		// τ (torque) = r × F (cross product of moment arm and force)
		//glm::vec3 r_cross_n_a = glm::cross(r_a, manifold.contact_normal);
		//glm::vec3 r_cross_n_b = glm::cross(r_b, manifold.contact_normal);

		//// How much does rotation contribute to velocity at contact point?
		//// When we apply torque, it changes ω (angular velocity)
		//// Change in velocity at point = (I^-1 * τ) × r
		//glm::vec3 ang_vel_effect_a = glm::cross(inv_inertia_world_a * r_cross_n_a, r_a);
		//glm::vec3 ang_vel_effect_b = glm::cross(inv_inertia_world_b * r_cross_n_b, r_b);

		//// "Effective mass" accounts for both linear AND angular motion
		//// It's how much the velocity changes per unit impulse
		//float effective_inv_mass =
		//	rb_a->inverse_mass + rb_b->inverse_mass +
		//	glm::dot(ang_vel_effect_a + ang_vel_effect_b, manifold.contact_normal);

		glm::vec3 raCn = glm::cross(r_a, manifold.contact_normal);
		glm::vec3 rbCn = glm::cross(r_b, manifold.contact_normal);

		float angular_term =
			glm::dot(
				glm::cross(inv_inertia_world_a * raCn, r_a) +
				glm::cross(inv_inertia_world_b * rbCn, r_b),
				manifold.contact_normal
			);

		float effective_inv_mass =
			rb_a->inverse_mass +
			rb_b->inverse_mass +
			angular_term;

		// The impulse magnitude (scalar)
		// Formula: j = -(1 + e) * v_rel / effective_mass
		float impulse_magnitude =
			-(1.0f + restitution) * velocity_along_normal / effective_inv_mass;

		// Convert to vector (direction = contact normal)
		glm::vec3 normal_impulse = impulse_magnitude * manifold.contact_normal;

		float contact_weight = 1.0f / float(manifold.num_contact_points);
		normal_impulse *= contact_weight;

		// --------------------------------------
		// 2.5: Apply Normal Impulse
		// --------------------------------------
		// An impulse is a change in momentum: Δp = F * Δt
		// Change in velocity: Δv = Δp / m = impulse * inverse_mass

		// LINEAR: Apply impulse to linear velocity
		rb_a->linear_velocity -= normal_impulse * rb_a->inverse_mass;
		rb_b->linear_velocity += normal_impulse * rb_b->inverse_mass;

		// ANGULAR: Apply torque to angular velocity
		// Torque: τ = r × impulse
		// Change in angular velocity: Δω = I^-1 * τ
		rb_a->angular_velocity -= inv_inertia_world_a * glm::cross(r_a, normal_impulse);
		rb_b->angular_velocity += inv_inertia_world_b * glm::cross(r_b, normal_impulse);

		// --------------------------------------
		// 2.6: Calculate and Apply Friction Impulse
		// --------------------------------------
		// Friction opposes TANGENTIAL motion (sliding).
		// It acts perpendicular to the normal.

		// Recalculate velocities after normal impulse
		velocity_a = rb_a->linear_velocity + glm::cross(rb_a->angular_velocity, r_a);
		velocity_b = rb_b->linear_velocity + glm::cross(rb_b->angular_velocity, r_b);
		relative_velocity = velocity_b - velocity_a;

		// Remove the normal component to get the tangent (sliding) component
		// tangent = total - normal_component
		// normal_component = (v · n) * n  (projection onto normal)
		glm::vec3 tangent_velocity =
			relative_velocity -
			manifold.contact_normal * glm::dot(relative_velocity, manifold.contact_normal);

		float tangent_speed = glm::length(tangent_velocity);

		if (tangent_speed > 0.0001f) // Avoid division by zero
		{
			// Direction of friction (opposes sliding)
			glm::vec3 tangent_direction = tangent_velocity / tangent_speed;

			// Calculate friction impulse (same process as normal impulse)
			glm::vec3 r_cross_t_a = glm::cross(r_a, tangent_direction);
			glm::vec3 r_cross_t_b = glm::cross(r_b, tangent_direction);

			glm::vec3 ang_vel_effect_t_a = glm::cross(inv_inertia_world_a * r_cross_t_a, r_a);
			glm::vec3 ang_vel_effect_t_b = glm::cross(inv_inertia_world_b * r_cross_t_b, r_b);

			float effective_inv_mass_tangent =
				rb_a->inverse_mass + rb_b->inverse_mass +
				glm::dot(ang_vel_effect_t_a + ang_vel_effect_t_b, tangent_direction);

			// Friction tries to stop all tangential motion
			float friction_impulse_magnitude =
				-glm::dot(relative_velocity, tangent_direction) / effective_inv_mass_tangent;

			// COULOMB FRICTION LAW: Friction force ≤ μ * Normal force
			// μ (mu) = friction coefficient
			// The friction impulse can't exceed friction * normal_impulse
			//float max_friction = friction * impulse_magnitude;

			float max_friction = friction * glm::length(normal_impulse);
			friction_impulse_magnitude = glm::clamp(
				friction_impulse_magnitude,
				-max_friction,
				max_friction
			);

			glm::vec3 friction_impulse = friction_impulse_magnitude * tangent_direction;

			// Apply friction impulse (same as normal impulse, different direction)
			rb_a->linear_velocity -= friction_impulse * rb_a->inverse_mass;
			rb_b->linear_velocity += friction_impulse * rb_b->inverse_mass;

			rb_a->angular_velocity -= inv_inertia_world_a * glm::cross(r_a, friction_impulse);
			rb_b->angular_velocity += inv_inertia_world_b * glm::cross(r_b, friction_impulse);
		}
	}
}

bool AABBReadSpatialPartitionSystem::execute(SystemsContext& ctx)
{
	if (counter > 5000)
	{
		counter = 0;
		using SpatialPartition = std::unordered_map<CellCoordinates, std::vector<entt::entity>>;
		if (SystemsStorageObject<SpatialPartition>* spatial_object = ctx.get_system_storage().get_object<SpatialPartition>("AABBSpatialPartition"))
		{
			spatial_object->read([](const SpatialPartition& spatial_partition)
				{
					for (const auto& [cell, entities] : spatial_partition)
					{
						for (const auto& entity : entities)
						{
							PRINTLN("entity {} in cell {} {} {}", (uint32_t)entity, cell.x, cell.y, cell.z);
						}
					}
				});
		}

	}
	else
	{
		++counter;
	}
	return false;
}

bool BoardPhaseCollision::execute(SystemsContext& ctx)
{
	//using SpatialPartition = std::unordered_map<CellCoordinates, std::vector<entt::entity>>;
	//if (auto spatial_partition_object = ctx.get_system_storage().get_object<SpatialPartition>("AABBSpatialPartition"))
	//{
	//	;
	//}
	
	return false;
}

bool SpawnCubeSystem::execute(SystemsContext& ctx)
{
	if (!has_spawned)
	{
		if (auto ecb = ctx.get_ecb("end_frame_ecb"))
		{
			ecb->create_entity(new_entity()
				.with_component<RenderComponent>(hashed_string_64("./assets/obj/cube.obj"), hashed_string_64("./assets/shaders/white.mat"), false, true, true)
				.with_component<TransformDynamicComponent>()
				.with_child(new_entity()
					.with_component<RenderComponent>(hashed_string_64("./assets/obj/cube.obj"), hashed_string_64("./assets/shaders/white.mat"), false, true, true)
					.with_component<TransformDynamicComponent>(TransformDynamicComponent{ glm::vec3(-1,-1,-1) })
					.assemble(), "new_child")
				.assemble(), "new_entity");
			ctx.execute_ecb("end_frame_ecb");
			has_spawned = true;
		}
	}
	//ctx.each(WithRead<EntityComponent>{},
	//	[](const entt::entity, const EntityComponent& comp)
	//	{
	//		PRINTLN("entity({}) name: {}",(uint32_t)comp.entity.entity, comp.hashed_name.string);
	//	});
	return false;
}

bool ECBCreatorSystem::execute(SystemsContext& ctx)
{
	ctx.create_ecb("end_frame_ecb");
	set_enabled(false);
	return false;
}

bool DestroyCubeSystem::execute(SystemsContext& ctx)
{
	if (!has_destroyed)
	{
		if (auto ecb = ctx.get_ecb("end_frame_ecb"))
		{
			ctx.each(
				WithRead<RenderComponent>{},
				[&ecb](const entt::entity entity, const RenderComponent& render_comp)
				{
					ecb->destroy_entity(entity);
					PRINTLN("entity({}) queued for destroy", (uint32_t)entity);
				}
			);
			has_destroyed = true;
			ctx.execute_ecb("end_frame_ecb");
		}
	}
	return false;
}

bool HeightMapLoadSystem::execute(SystemsContext& ctx)
{
	ctx.enqueue_each(WithWrite<LandscapeComponent>{},
		[this, &ctx](const entt::entity entity, LandscapeComponent& landscape)
		{
			if (!landscape.has_loaded_height_map)
			{
				auto& system_storage = ctx.get_system_storage();
				HeightMap map;
				map.max_height = landscape.max_height;
				if (generate_heightmap(landscape.height_map_path.string, map))
				{
					landscape.has_loaded_height_map;
					system_storage.add_or_replace_object(landscape.height_map_path.string, map);
				}
			}
		});
	return false;
}

bool HeightMapLoadSystem::generate_heightmap(const std::string& path, HeightMap& map)
{

	int width, height, channels;
	
	stbi_set_flip_vertically_on_load(true);
	unsigned char* height_data = stbi_load(path.c_str(), &width, &height, &channels, 0);
	if (!height_data || width < 0 || height < 0) { return false; }
	map.height_map.clear();
	map.height_map.reserve((size_t)width * (size_t)height);
	map.height = height;
	map.width = width;
	float max_height = map.max_height;
	for (int u = 0; u < width; u++)
	{
		for (int v = 0; v < height; v++)
		{
			int index = (v * width + u) * channels;
			unsigned char red = height_data[index];
			float height = ((float)red / 255.f) * max_height;
			map.height_map.push_back(height);
		}
	}
	return true;
}
