#include <engine/game/systems.h>
#include <engine/game/components.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>
#include <engine/utilities/macros.h>
#include <engine/resources/data_structs.h>
#include <stb_image/stb_image.h>
#include <glm/gtc/matrix_transform.hpp>  
#include <engine/core/game_thread.h>
#include <engine/utilities/layer_masks.h>

bool MoveSystem::execute(SystemsContext& ctx)
{
	
	time += ctx.get_delta_time();
	float current_time = time;

	return ctx.enqueue_parallel_each(
		WithRead<SpawnPositionComponent, RenderComponent>{},
		WithWrite<TransformDynamicComponent>{},
		[current_time](
			const entt::entity entity, 
			const SpawnPositionComponent& spawn, 
			const RenderComponent& render, 
			TransformDynamicComponent& transform
			)
		{
			float amplitude = 5.f;
			float frequency = 5.f;
			float offset = amplitude * glm::sin(frequency * transform.position.x + current_time);
			transform.position.y = spawn.spawn_position.y + offset;
		}, 512);
}

bool TransformCalculationSystem::execute(SystemsContext& ctx)
{
	size_t num_threads = ctx.num_threads() * 4;
	roots.clear();
	roots.resize(num_threads);
	for (auto& root : roots)
	{
		root.clear();
		root.reserve(1000);
	}
	ctx.enqueue_parallel_data_each(
		WithRead<HierarchyComponent>{},
		WithWrite<TransformDynamicComponent>{},
		[](std::vector<RootHierachyTransform>& roots,
			const entt::entity entity,
			const HierarchyComponent hierachy,
			TransformDynamicComponent& transform)
		{
			transform.calculate_model_matrix();
			if (hierachy.parent.entity == entt::null && hierachy.child.entity != entt::null)
				roots.emplace_back(entity, transform.model_matrix);
		}, roots, num_threads);
	ctx.entt_sync();

	//for (size_t thread_id = 0; thread_id < num_threads; ++thread_id)
	//{
	//	ctx.enqueue([this, &ctx, thread_id]()
	//		{
	//			std::vector<RootHierachyTransform> stack;
	//			stack.reserve(10);

	//			for (const auto& root : roots[thread_id])
	//			{
	//				auto* root_t = ctx.try_get<TransformDynamicComponent>(root.parent);
	//				if (!root_t) continue;

	//				// push its first child
	//				auto* root_h = ctx.try_get<HierarchyComponent>(root.parent);
	//				if (!root_h || root_h->child.entity == entt::null) continue;

	//				stack.push_back({ root_h->child.entity, root_t->model_matrix });

	//				while (!stack.empty())
	//				{
	//					auto [entity, parent_world] = stack.back();
	//					stack.pop_back();

	//					auto* t = ctx.try_get<TransformDynamicComponent>(entity);
	//					auto* h = ctx.try_get<HierarchyComponent>(entity);
	//					if (!t || !h) continue;

	//					// t->model_matrix was written as local in phase 2
	//					// now bake in parent
	//					t->model_matrix = parent_world * t->model_matrix;

	//					// push next sibling onto stack (same parent_world)
	//					if (h->next_sibling.entity != entt::null)
	//						stack.push_back({ h->next_sibling.entity, parent_world });

	//					// push first child (our freshly computed world is its parent)
	//					if (h->child.entity != entt::null)
	//						stack.push_back({ h->child.entity, t->model_matrix });
	//				}
	//			}
	//		});
	//}

	ctx.enqueue_parallel(
		WithWrite<TransformDynamicComponent, HierarchyComponent>{},
		[this, &ctx](size_t thread_id)
		{
			std::vector<RootHierachyTransform> stack;
			stack.reserve(10);

			for (const auto& root : roots[thread_id])
			{
				auto* root_t = ctx.try_get<TransformDynamicComponent>(root.parent);
				if (!root_t) continue;

				// push its first child
				auto* root_h = ctx.try_get<HierarchyComponent>(root.parent);
				if (!root_h || root_h->child.entity == entt::null) continue;

				stack.push_back({ root_h->child.entity, root_t->model_matrix });

				while (!stack.empty())
				{
					auto [entity, parent_world] = stack.back();
					stack.pop_back();

					auto* t = ctx.try_get<TransformDynamicComponent>(entity);
					auto* h = ctx.try_get<HierarchyComponent>(entity);
					if (!t || !h) continue;

					// t->model_matrix was written as local in phase 2
					// now bake in parent
					t->model_matrix = parent_world * t->model_matrix;

					// push next sibling onto stack (same parent_world)
					if (h->next_sibling.entity != entt::null)
						stack.push_back({ h->next_sibling.entity, parent_world });

					// push first child (our freshly computed world is its parent)
					if (h->child.entity != entt::null)
						stack.push_back({ h->child.entity, t->model_matrix });
				}
			}
		}, num_threads);
	//ctx.gen_sync();

	return true;
}

void TransformCalculationSystem::calculate_model_matrix(TransformDynamicComponent& transform)
{
	if (transform.overide_quaternion)
	{
		transform.rotation_euler_do_not_use.x = std::fmod(transform.rotation_euler_do_not_use.x, 360.0f);
		if (transform.rotation_euler_do_not_use.x < 0.0f) transform.rotation_euler_do_not_use.x += 360.0f;
		transform.rotation_euler_do_not_use.y = std::fmod(transform.rotation_euler_do_not_use.y, 360.0f);
		if (transform.rotation_euler_do_not_use.y < 0.0f) transform.rotation_euler_do_not_use.y += 360.0f;
		transform.rotation_euler_do_not_use.z = std::fmod(transform.rotation_euler_do_not_use.z, 360.0f);
		if (transform.rotation_euler_do_not_use.z < 0.0f) transform.rotation_euler_do_not_use.z += 360.0f;

		transform.rotation_quat = glm::quat(glm::radians(transform.rotation_euler_do_not_use));
	}
	if (transform.get_euler_angles) /// 0.5ms
	{
		transform.rotation_euler_do_not_use = glm::degrees(glm::eulerAngles(transform.rotation_quat));
		//if (world_pos.overide_quaternion)
		//	world_pos.get_euler_angles = false;
	}

	transform.model_matrix = glm::translate(glm::mat4(1.0f), transform.position); // 0.47ms
	transform.model_matrix *= glm::mat4_cast(transform.rotation_quat); // 1.47ms
	transform.model_matrix = glm::scale(transform.model_matrix, transform.scale); // 0.4ms
}

bool PhysicsSystem::execute(SystemsContext& ctx)
{
	timer.start();
	size_t num_threads = ctx.num_threads();

	// consider just storing pointer to this in the storage
	vectored_spatial_collider_partitioning.clear();
	vectored_spatial_collider_partitioning.resize(num_threads);

	count++;
	if (count > 100)
	{
		PRINTLN("launching threads");

	}


	{
		std::vector<std::pair<size_t, size_t>> sizes;
		sizes.resize(num_threads);
		{
			size_t index = 0;
			for (auto& cell_entry : vectored_cell_entries)
			{
				sizes[index++] = { cell_entry.small_entries.size(), cell_entry.massive_entries.size() };
			}
		}


		vectored_cell_entries.clear();
		vectored_cell_entries.resize(num_threads);
		{
			size_t index = 0;
			for (auto& cell_entry : vectored_cell_entries)
			{
				cell_entry.small_entries.reserve(sizes[index].first);
				cell_entry.massive_entries.reserve(sizes[index].second);
				++index;
			}
		}
	}

	ctx.enqueue_parallel_data_each(
		WithRead<TransformDynamicComponent>{},
		WithWrite<BoundingBoxComponent>{},
		WithOut<LandscapeComponent>{},
		WithRef<RigidBodyComponent>{},
		[this, &ctx](CellEntry& cell_entires,
			const entt::entity entity,
			const TransformDynamicComponent& transform,
			BoundingBoxComponent& bbox,
			size_t access_order)
		{
			auto& small_entries = cell_entires.small_entries;
			auto& massive_entries = cell_entires.massive_entries;

			const glm::mat4& model_matrix = transform.model_matrix;
			bbox.has_collision_info = false;

			
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

			glm::mat3 abs_rotation(
				glm::abs(rotation[0]),
				glm::abs(rotation[1]),
				glm::abs(rotation[2])
			);

			glm::vec3 world_half_extents = abs_rotation * half_extents;

			glm::vec3 aabb_min = offset_rot_scaled_box - world_half_extents;
			glm::vec3 aabb_max = offset_rot_scaled_box + world_half_extents;
			AABB aabb{ aabb_min, aabb_max };
			OBB obb{ offset_rot_scaled_box, obb_rotation, half_extents };
			PhysicsLayers physics_layers{ 0, 0, bbox.collision_layers };

			auto rb = ctx.try_get<RigidBodyComponent>(entity);
			bool asleep = false;
			if (rb)
			{
				if (rb->recalculate_inverse_inertia_tensor)
				{
					rb->recalculate_inverse_inertia_tensor = false;
					rb->inverse_inertia_tensor_local = calculate_box_inertia_tensor_inverse(rb->mass, half_extents);
				}
				physics_layers.static_layers = rb->static_layers;
				physics_layers.dynamic_layers = rb->dynamic_layers;
				asleep = rb->asleep;
			}
			CellCoordinates min_cell = {
				static_cast<int>(divide_int(aabb_min.x, cell_size)),
				static_cast<int>(divide_int(aabb_min.y, cell_size)),
				static_cast<int>(divide_int(aabb_min.z, cell_size))
			};

			CellCoordinates max_cell = {
				static_cast<int>(divide_int(aabb_max.x, cell_size)),
				static_cast<int>(divide_int(aabb_max.y, cell_size)),
				static_cast<int>(divide_int(aabb_max.z, cell_size))
			};
			if ((max_cell.x - min_cell.x) * (max_cell.y - min_cell.y) * (max_cell.z - min_cell.z) > max_cells)
			{
				// register to large partition instead.
				massive_entries.emplace_back(entity, access_order, rb, aabb, obb, physics_layers, asleep, bbox.tag);
			}
			else
			{
				for (int x = min_cell.x; x <= max_cell.x; ++x)
					for (int y = min_cell.y; y <= max_cell.y; ++y)
						for (int z = min_cell.z; z <= max_cell.z; ++z)
						{
							small_entries.emplace_back(CellCoordinates(x, y, z),
								entity, access_order, rb,
								aabb, obb, physics_layers, asleep, bbox.tag);
						}

			}
			// really need to make this into std::pair<cell, other> in a vector instead of unordered_map

		},vectored_cell_entries, num_threads, true);
	ctx.entt_sync();

	if (count > 100)
	{
		size_t thread_id = 0;
		for (auto& cell_entry : vectored_cell_entries)
		{
			PRINTLN("thread({}) - found entries: {}", thread_id, cell_entry.small_entries.size());
		}
		
	}

	CellEntry merged_cell_entries;
	{
		size_t total_small = 0;
		size_t total_massive = 0;
		for (auto& cell_entry : vectored_cell_entries)
		{
			total_small += cell_entry.small_entries.size();
			total_massive += cell_entry.massive_entries.size();
		}
		merged_cell_entries.small_entries.reserve(total_small);
		merged_cell_entries.massive_entries.reserve(total_massive);
	}

	for (size_t i = 0; i < vectored_cell_entries.size(); i++) {
		if (vectored_cell_entries[i].small_entries.size() > 0)
		{
			merged_cell_entries.small_entries.insert(merged_cell_entries.small_entries.end(),
				std::make_move_iterator(vectored_cell_entries[i].small_entries.begin()),
				std::make_move_iterator(vectored_cell_entries[i].small_entries.end()));
		}
		if (vectored_cell_entries[i].massive_entries.size() > 0)
		{
			merged_cell_entries.massive_entries.insert(merged_cell_entries.massive_entries.end(),
				std::make_move_iterator(vectored_cell_entries[i].massive_entries.begin()),
				std::make_move_iterator(vectored_cell_entries[i].massive_entries.end()));
		}
	}


	std::sort(merged_cell_entries.small_entries.begin(),
		merged_cell_entries.small_entries.end());

	//std::vector<CellEntry> chunks;
	std::vector<StartStop> partitioned_entries;
	partitioned_entries.reserve(merged_cell_entries.small_entries.size());
	{
		size_t i = 0;
		if (!merged_cell_entries.small_entries.empty())
		{
			auto& small_entries = merged_cell_entries.small_entries;
			CellCoordinates cell_coord = small_entries[0].coordinates;
			size_t chunk_index = 0;
			//chunks.emplace_back();
			//chunks[chunk_index].small_entries.reserve(10);
			//size_t chunk_max = 512;
			//size_t chunk_count = 0;
			size_t start = 0;
			partitioned_entries.push_back({0 ,0});
			for (size_t i = 0; i < small_entries.size(); i++)
			{
				if (cell_coord != small_entries[i].coordinates)
				{
					partitioned_entries.back().stop = i;
					//chunks.emplace_back(); // this is pretty bad, I should rather just record the "cut off"
					//chunks[++chunk_index].small_entries.reserve(10);
					cell_coord = small_entries[i].coordinates;
					//chunk_count = 0;

					partitioned_entries.push_back({i, 0}); // this should be better
				}
				//chunks[chunk_index].small_entries.push_back(std::move(small_entries[i]);
			}
			partitioned_entries.back().stop = small_entries.size();
		}
	}

	timer.stop();
	timer_spatial += timer.get_time_us();
	timer.start();
	landscape_manifolds.clear();
	landscape_rbs.clear();
	CellEntry new_merged_cell_entries;
	new_merged_cell_entries.small_entries.resize(merged_cell_entries.small_entries.size());
	new_merged_cell_entries.massive_entries.resize(merged_cell_entries.massive_entries.size());
	std::memcpy(
		new_merged_cell_entries.small_entries.data(), 
		merged_cell_entries.small_entries.data(), 
		merged_cell_entries.small_entries.size() 
		* sizeof(SmallEntry));

	std::memcpy(
		new_merged_cell_entries.massive_entries.data(),
		merged_cell_entries.massive_entries.data(),
		merged_cell_entries.massive_entries.size()
		* sizeof(MassiveEntry));

	landscape_rbs.reserve(merged_cell_entries.small_entries.size() + merged_cell_entries.massive_entries.size());
	landscape_manifolds.reserve(merged_cell_entries.small_entries.size() + merged_cell_entries.massive_entries.size());
	// here I can start the landscape stuff ... single threaded? Probably
	
	// should maybe make it parallel data each instead, but I don't care anymore, I just want this pain to end

	std::vector<CollisionResult> land_col_res;
	land_col_res.reserve(merged_cell_entries.small_entries.size() + merged_cell_entries.massive_entries.size());
	size_t landscapes = 0;
	ctx.enqueue_each(
		WithRead<const TransformDynamicComponent, const LandscapeComponent>{},
		[this, &new_merged_cell_entries, &landscapes, &land_col_res](
			const entt::entity entity, 
			const TransformDynamicComponent& transform,
			const LandscapeComponent& landscape)
		{
			landscapes++;
			if (!landscape.has_loaded_height_map) return;
			AABB landscape_aabb; 
			const auto& map = landscape.map;
			if (map.height_map.empty()) return;
			landscape_aabb.aabb_min = (transform.position - (transform.scale * 0.5f));
			landscape_aabb.aabb_min.y = transform.position.y;
			landscape_aabb.aabb_max = (transform.position + (transform.scale * 0.5f));
			landscape_aabb.aabb_max.y = transform.position.y + landscape.max_height;

			float cell_x = transform.scale.x / (map.width - 1);
			float cell_z = transform.scale.z / (map.height - 1);


			const auto& small_entries = new_merged_cell_entries.small_entries;
			const auto& massive_entries = new_merged_cell_entries.massive_entries;

			for (size_t i = 0; i < small_entries.size(); i++)
			{
				const auto& entry = small_entries[i];
				//glm::bvec3 nans = glm::isnan(entry.aabb.aabb_max);
				//if (nans.x && nans.y && nans.z)
				//{
				//	PRINTLN("NAN detected");
				//}
				bool _overlap = overlap(landscape_aabb, entry.aabb);
				if (_overlap)
				{

					glm::vec3 local_min = entry.aabb.aabb_min - landscape_aabb.aabb_min;
					glm::vec3 local_max = entry.aabb.aabb_max - landscape_aabb.aabb_min;


					float norm_min_x = local_min.x / transform.scale.x;
					float norm_max_x = local_max.x / transform.scale.x;
					float norm_min_z = local_min.z / transform.scale.z;
					float norm_max_z = local_max.z / transform.scale.z;

					size_t start_x = (size_t)glm::clamp(norm_min_x * (map.width - 1), 0.0f, (float)(map.width - 1));
					size_t end_x = (size_t)glm::clamp(norm_max_x * (map.width - 1), 0.0f, (float)(map.width - 1));
					size_t start_z = (size_t)glm::clamp(norm_min_z * (map.height - 1), 0.0f, (float)(map.height - 1));
					size_t end_z = (size_t)glm::clamp(norm_max_z * (map.height - 1), 0.0f, (float)(map.height - 1));

					start_x = (size_t)std::max(((int)start_x - 1), 0);
					start_z = (size_t)std::max(((int)start_z - 1), 0);
					end_x = (size_t)std::min(((int)end_x + 1), ((int)map.width - 1));
					end_z = (size_t)std::min(((int)end_z + 1), ((int)map.height -1));
					if ((start_x == end_x) && (start_z = end_z))
					{
						continue;
					}

					// Step 4: Find max height in this region
					float max_height = -1;
					float max_height2 = -1;
					float max_height3 = -1;
					glm::ivec2 max_height_xz = glm::ivec2(-1);
					glm::ivec2 max_height2_xz = glm::ivec2(-1);
					glm::ivec2 max_height3_xz = glm::ivec2(-1);
					// do I really want this? I guess...
					// nah, lets just sample neighboring ones + outer shell
					for (size_t x = start_x; x <= end_x; x++)
					{
						for (size_t z = start_z; z <= end_z; z++)
						{
							size_t index = z * map.width + x;
							if (index >= map.height_map.size())
							{
								continue;
							}
							//if (map.height_map[index] * landscape.max_height > max_height)
							//{
							//	max_height3 = max_height2;
							//	max_height2 = max_height;
							//	max_height = map.height_map[index] * landscape.max_height;
							//	max_height3_xz = max_height2_xz;
							//	max_height2_xz = max_height_xz;
							//	max_height_xz = glm::ivec2(x, z);
							//}
							//else if (map.height_map[index] * landscape.max_height > max_height2)
							//{
							//	max_height3 = max_height2;
							//	max_height2 = map.height_map[index] * landscape.max_height;
							//	max_height3_xz = max_height2_xz;
							//	max_height2_xz = glm::ivec2(x, z);
							//}
							//else if ((map.height_map[index] * landscape.max_height > max_height3))
							//{
							//	max_height3 = map.height_map[index] * landscape.max_height;;
							//	max_height3_xz = glm::ivec2(x, z);
							//}
							max_height = std::max(max_height, map.height_map[index] * landscape.max_height); // or however you access heights
						}
					}
					if (max_height >= 0 && entry.aabb.aabb_min.y < (transform.position.y + max_height + 10.f))
					{
						glm::vec3 entry_abb_center = ((entry.aabb.aabb_min + entry.aabb.aabb_max) * 0.5f);
						glm::vec3 local_center = entry_abb_center - landscape_aabb.aabb_min;
						float norm_x = local_center.x / transform.scale.x;
						float norm_z = local_center.z / transform.scale.z;

						size_t closest_x = (size_t)glm::clamp(norm_x * (map.width - 1), 0.0f, (float)(map.width - 1));
						size_t closest_z = (size_t)glm::clamp(norm_z * (map.height - 1), 0.0f, (float)(map.height - 1));

						// actually lets do these + the outer corners and average
						// no just do the whole plane instead

						// sample 4 height points
						float height_center = map.height_map[closest_z * map.width + closest_x];

						float height_ul = map.height_map[start_x + map.width * start_z];
						float height_ur = map.height_map[end_x + map.width * start_z];
						
						float height_ll = map.height_map[start_x + map.width * end_z];
						float height_lr = map.height_map[end_x + map.width * end_z];

						// create the points
						glm::vec3 origin = landscape_aabb.aabb_min;

						glm::vec3 p_ul = glm::vec3(cell_x * start_x, height_ul, cell_z * start_z);
						glm::vec3 p_ur = glm::vec3(cell_x * end_x, height_ur, cell_z * start_z);
						glm::vec3 p_ll = glm::vec3(cell_x * start_x, height_ll, cell_z * end_z);
						glm::vec3 p_lr = glm::vec3(cell_x * end_x, height_lr, cell_z * end_z);

						// get normals
						// even if I get this wrong, it should be almost right, and visible ... apparently not..
						//glm::vec3 n_1 = glm::cross(p_ur - p_ul, p_ll - p_ul);
						//glm::vec3 n_2 = glm::cross(p_lr - p_ur, p_ll - p_ur);
						//glm::vec3 plane_normal = glm::normalize(n_1 + n_2);

						glm::vec3 n_1 = glm::cross(p_ll - p_ul, p_ur - p_ul);
						glm::vec3 n_2 = glm::cross(p_ll - p_ur, p_lr - p_ur);
						glm::vec3 plane_normal = glm::normalize(n_1 + n_2);

						glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
						
						glm::vec3 normal = glm::normalize(plane_normal);
						if (plane_normal.y < 0) {
							plane_normal = -plane_normal;
								PRINTLN("Flipped normals");
						}

						glm::vec3 tangent = glm::normalize(glm::cross(up, normal));
						glm::vec3 bitangent = glm::cross(normal, tangent);


						glm::mat3 rot;
						rot[0] = tangent;
						rot[1] = normal;
						rot[2] = bitangent;

						glm::quat rotation = glm::normalize(glm::quat_cast(rot));
						glm::quat quat_up = glm::quat(1, 0, 0, 0);

						float patch_width = (end_x - start_x + 1) * cell_x;
						float patch_depth = (end_z - start_z + 1) * cell_z;

						glm::vec3 center = entry_abb_center;
						center.y = transform.position.y + height_center;


						glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f);
						float dot = glm::dot(world_up, plane_normal);
						if (glm::abs(dot) > 0.99f) {
							rotation = quat_up;
						}
						else {
							glm::vec3 axis = glm::normalize(glm::cross(world_up, plane_normal));
							constexpr float max_angle = glm::radians(85.f);
							float angle = glm::acos(glm::clamp(dot, -1.0f, 1.0f));
							angle = glm::min(angle, max_angle);
							rotation = glm::angleAxis(angle, axis);
						}
						center = center - (rotation * up) * 5.f;
						OBB obb{ center, rotation, glm::vec3(patch_width * 125.f, 5.f, patch_depth * 125.f)};

						
						RigidBodyComponent fake_rb;
						fake_rb.position = center;
						fake_rb.rotation = rotation;
						fake_rb.inverse_mass = 0.0f;
						fake_rb.linear_velocity = glm::vec3(0);
						fake_rb.angular_velocity = glm::vec3(0);
						// or these 
						fake_rb.friction = 0.4f;
						fake_rb.restitution = 0.9f;
						// I think these two were corrupting memory somehow
						fake_rb.static_layers = landscape.static_layers;
						fake_rb.dynamic_layers = 0;

						//fake_rb.is = true;

						landscape_rbs.push_back(fake_rb);
						size_t rb_index = landscape_rbs.size() - 1;

						PhysicsLayers landscape_layer(landscape.static_layers, landscape.dynamic_layers, landscape.collision_layers);
						BroadPhasePair pair(entry.entity, entt::null, entry.access_order, 0, entry.rb, &landscape_rbs.back(),
							entry.obb, obb, entry.layers, landscape_layer, false, true);
						ContactManifold manifold = OBB_collision_test(pair);
						if (manifold.has_collision)
						{
							if (entry.layers.is_collision_against(landscape_layer.collision_layers))
							{
								CollisionResult a;
								a.self_entity = entry.entity;
								a.other_entity = entt::null;
								a.self_access_order = entry.access_order;
								a.other_access_order = entry.access_order;
								a.other_tag = landscape.tag;
								land_col_res.push_back(a); // need to put in another collision results
							}
							manifold.landscape_index = rb_index;
							landscape_manifolds.push_back(manifold);
						}
					}
				}
			}

			for (size_t i = 0; i < massive_entries.size(); i++)
			{
				const auto& entry = massive_entries[i];
				bool _overlap = overlap(landscape_aabb, entry.aabb);
				if (_overlap)
				{

					glm::vec3 local_min = entry.aabb.aabb_min - landscape_aabb.aabb_min;
					glm::vec3 local_max = entry.aabb.aabb_max - landscape_aabb.aabb_min;

					float norm_min_x = local_min.x / transform.scale.x; 
					float norm_max_x = local_max.x / transform.scale.x;
					float norm_min_z = local_min.z / transform.scale.z;
					float norm_max_z = local_max.z / transform.scale.z;

					size_t start_x = (size_t)glm::clamp(norm_min_x * (map.width - 1), 0.0f, (float)(map.width - 1));
					size_t end_x = (size_t)glm::clamp(norm_max_x * (map.width - 1), 0.0f, (float)(map.width - 1));
					size_t start_z = (size_t)glm::clamp(norm_min_z * (map.height - 1), 0.0f, (float)(map.height - 1));
					size_t end_z = (size_t)glm::clamp(norm_max_z * (map.height - 1), 0.0f, (float)(map.height - 1));

					start_x = (size_t)std::max(((int)start_x - 1), 0);
					start_z = (size_t)std::max(((int)start_z - 1), 0);
					end_x = (size_t)std::min(((int)end_x + 1), ((int)map.width - 1));
					end_z = (size_t)std::min(((int)end_z + 1), ((int)map.height - 1));
					if ((start_x == end_x) && (start_z = end_z))
					{
						continue;
					}

					// Step 4: Find max height in this region
					float max_height = -1;
					// do I really want this? I guess...
					// nah, lets just sample neighboring ones + outer shell
					for (size_t x = start_x; x <= end_x; x++)
					{
						for (size_t z = start_z; z <= end_z; z++)
						{
							size_t index = z * map.width + x;
							if (index >= map.height_map.size())
							{
								continue;
							}
							max_height = std::max(max_height, map.height_map[index] * landscape.max_height); // or however you access heights
						}
					}
					if (max_height >= 0 && entry.aabb.aabb_min.y < (transform.position.y + max_height + 10.f))
					{
						// Find the closest grid point to calculate slope
						glm::vec3 entry_abb_center = ((entry.aabb.aabb_min + entry.aabb.aabb_max) * 0.5f);
						glm::vec3 local_center = entry_abb_center - landscape_aabb.aabb_min;
						float norm_x = local_center.x / transform.scale.x;
						float norm_z = local_center.z / transform.scale.z;

						size_t closest_x = (size_t)glm::clamp(norm_x * (map.width - 1), 0.0f, (float)(map.width - 1));
						size_t closest_z = (size_t)glm::clamp(norm_z * (map.height - 1), 0.0f, (float)(map.height - 1));

						// actually lets do these + the outer corners and average
						// no just do the whole plane instead

						// sample 4 height points
						float height_center = map.height_map[closest_z * map.width + closest_x];

						float height_ul = map.height_map[start_x + map.width * start_z];
						float height_ur = map.height_map[end_x + map.width * start_z];

						float height_ll = map.height_map[start_x + map.width * end_z];
						float height_lr = map.height_map[end_x + map.width * end_z];

						// create the points
						glm::vec3 origin = landscape_aabb.aabb_min;

						glm::vec3 p_ul = glm::vec3(cell_x * start_x, height_ul, cell_z * start_z);
						glm::vec3 p_ur = glm::vec3(cell_x * end_x, height_ur, cell_z * start_z);
						glm::vec3 p_ll = glm::vec3(cell_x * start_x, height_ll, cell_z * end_z);
						glm::vec3 p_lr = glm::vec3(cell_x * end_x, height_lr, cell_z * end_z);

						glm::vec3 n_1 = glm::cross(p_ll - p_ul, p_ur - p_ul);  // Flipped
						glm::vec3 n_2 = glm::cross(p_ll - p_ur, p_lr - p_ur);  // Flipped
						glm::vec3 plane_normal = glm::normalize(n_1 + n_2);

						glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

						glm::vec3 normal = glm::normalize(plane_normal);
						if (plane_normal.y < 0) {
							plane_normal = -plane_normal;
							PRINTLN("Flipped normals");
						}

						glm::vec3 tangent = glm::normalize(glm::cross(up, normal));
						glm::vec3 bitangent = glm::cross(normal, tangent);
						glm::mat3 rot;
						rot[0] = tangent;
						rot[1] = normal;
						rot[2] = bitangent;

						glm::quat rotation = glm::normalize(glm::quat_cast(rot));
						glm::quat quat_up = glm::quat(1, 0, 0, 0);

						float patch_width = (end_x - start_x + 1) * cell_x;
						float patch_depth = (end_z - start_z + 1) * cell_z;

						glm::vec3 position = entry_abb_center;
						position.y = transform.position.y + height_center;


						glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f);
						float dot = glm::dot(world_up, plane_normal);
						if (glm::abs(dot) > 0.99f) {
							rotation = quat_up;
						}
						else {
							glm::vec3 axis = glm::normalize(glm::cross(world_up, plane_normal));
							constexpr float max_angle = glm::radians(85.f);
							float angle = glm::acos(glm::clamp(dot, -1.0f, 1.0f));
							angle = glm::min(angle, max_angle);
							rotation = glm::angleAxis(angle, axis);
						}
						position = position - (rotation * up) * 5.f;
						OBB obb{ position, rotation, glm::vec3(patch_width * 125.f, 5.f, patch_depth * 125.f) };

						RigidBodyComponent fake_rb;
						fake_rb.position = position;
						fake_rb.rotation = rotation;
						fake_rb.inverse_mass = 0.0f;
						fake_rb.linear_velocity = glm::vec3(0);
						fake_rb.angular_velocity = glm::vec3(0);
						// or these 
						fake_rb.friction = 0.4f;
						fake_rb.restitution = 0.9f;
						// I think these two were corrupting memory somehow
						fake_rb.static_layers = landscape.static_layers;
						fake_rb.dynamic_layers = 0;

						//fake_rb.is = true;

						// Add to vector and store index
						landscape_rbs.push_back(fake_rb);
						size_t rb_index = landscape_rbs.size() - 1;
						
						// Use existing OBB collision
						PhysicsLayers landscape_layer(landscape.static_layers, landscape.dynamic_layers, landscape.collision_layers);
						BroadPhasePair pair(entry.entity, entt::null, entry.access_order, 0, entry.rb, &landscape_rbs.back(),
							entry.obb, obb, entry.layers, landscape_layer, false, true);

						ContactManifold manifold = OBB_collision_test(pair);
						if (manifold.has_collision)
						{
							if (entry.layers.is_collision_against(landscape_layer.collision_layers))
							{
								CollisionResult a;
								a.self_entity = entry.entity;
								a.other_entity = entt::null;
								a.self_access_order = entry.access_order;
								a.other_access_order = entry.access_order;
								a.other_tag = landscape.tag;
								land_col_res.push_back(a); // need to put in another collision results
							}
							manifold.landscape_index = rb_index;
							landscape_manifolds.push_back(manifold);
						}
					}
				}
			}
		}
	);

	std::vector<std::vector<BroadPhasePair>> thread_pairs(num_threads);
	std::vector<std::vector<CollisionResult>> thread_col_res(num_threads);
	//const size_t chunks_per_thread = (chunks.size() + num_threads - 1) / num_threads;
	const size_t entries_per_thread = (partitioned_entries.size() + num_threads - 1) / num_threads;
	// would be nice to not start unecessary threads..

	for (size_t i = 0; i < num_threads; i++)
	{

		size_t thread_id = i;
		//size_t start_chunk = chunks_per_thread * thread_id;
		//size_t end_chunk = std::min(start_chunk + chunks_per_thread, chunks.size());
		size_t start_entry = entries_per_thread * thread_id;
		size_t end_entry = std::min(start_entry + entries_per_thread, partitioned_entries.size());
		ctx.enqueue(
			[this, &partitioned_entries, &merged_cell_entries, &thread_pairs, &thread_col_res, entries_per_thread, start_entry, end_entry, thread_id]()
			{
				auto& pairs = thread_pairs[thread_id];
				auto& col_res = thread_col_res[thread_id];

				//const auto& colliders = chunks[thread_id].small_entries;
				const auto& massives = merged_cell_entries.massive_entries;
				const auto& entries = partitioned_entries;
				const auto& colliders = merged_cell_entries.small_entries;
				pairs.reserve(colliders.size());
				col_res.reserve(colliders.size());
				size_t its = 0;
				//const auto& my_chunks = std::span(chunks).subspan(start_chunk, end_chunk - start_chunk);
				
				//for (const auto& chunk : my_chunks)
				for(size_t e = start_entry; e < end_entry; ++e)
				{
					if (end_entry  == 0)
					{
						break;
					}
					//const auto& colliders = chunk.small_entries;
					//for (size_t i = 0; i < colliders.size(); i++)
					
					for (size_t i = entries[e].start; i < entries[e].stop; ++i)
					{
						//for (size_t j = i + 1; j < colliders.size(); j++)
						for (size_t j = i+1; j < entries[e].stop; ++j)
						{
							//++its;
							// here we'de needto check "colliders" what they are.
							if (colliders[i].entity != colliders[j].entity
								&& (!colliders[i].asleep || !colliders[j].asleep)
								&& overlap(colliders[i].aabb, colliders[j].aabb)) // check if potential collision
							{
								if (colliders[i].layers.any_collision(colliders[j].layers) || true)
								{
									// collision
									//BroadPhasePair
									if ((uint32_t)colliders[i].entity < (uint32_t)colliders[j].entity)
									{
										pairs.emplace_back(colliders[i].entity, colliders[j].entity,
											colliders[i].access_order, colliders[j].access_order,
											colliders[i].rb, colliders[j].rb,
											colliders[i].obb, colliders[j].obb,
											colliders[i].layers, colliders[j].layers,
											colliders[i].tag, colliders[j].tag);
									}
									else
									{
										pairs.emplace_back(colliders[j].entity, colliders[i].entity,
											colliders[j].access_order, colliders[i].access_order,
											colliders[j].rb, colliders[i].rb,
											colliders[j].obb, colliders[i].obb,
											colliders[j].layers, colliders[i].layers,
											colliders[j].tag, colliders[i].tag);
									}
								}
								// I think here is where we generate new info for the landscape... call it entt::null
								// we'll need to create a new obb... but we need more information to make these decisions

							}
						}

						// test against every massive
						for (const auto& massive : massives)
						{
							//its++;
							if (colliders[i].entity != massive.entity
								&& (!colliders[i].asleep || !massive.asleep)
								&& overlap(colliders[i].aabb, massive.aabb)) // check if potential collision
							{
								//if (colliders[i].asleep && massive.layers.is_static_against(colliders[i].layers.dynamic_layers) &&
								//	massive.rb && glm::length(massive.rb->linear_velocity) < sleep_linear_threshold && glm::length(massive.rb->angular_velocity) < sleep_angular_threshold)
								//{
								//	continue;
								//}
								if (colliders[i].layers.any_collision(massive.layers) || true)
								{
									// collision
									//BroadPhasePair
									if ((uint32_t)colliders[i].entity < (uint32_t)massive.entity) // why? dumb of me
									{
										pairs.emplace_back(colliders[i].entity, massive.entity,
											colliders[i].access_order, massive.access_order,
											colliders[i].rb, massive.rb,
											colliders[i].obb, massive.obb,
											colliders[i].layers, massive.layers,
											colliders[i].tag, massive.tag);
									}
									else
									{
										pairs.emplace_back(massive.entity, colliders[i].entity,
											massive.access_order, colliders[i].access_order,
											massive.rb, colliders[i].rb,
											massive.obb, colliders[i].obb,
											massive.layers, colliders[i].layers,
											massive.tag, colliders[i].tag);
									}
								}
								// I think here is where we generate new info for the landscape... call it entt::null
								// we'll need to create a new obb... but we need more information to make these decisions

							}
						}
					}
				}
				
				// sort
				std::sort(pairs.begin(), pairs.end());
				// purge
				pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
				// replace
				//spatial_collider_partitioning.broad_phase_data = pairs;
				if (count > 100)
				{
					PRINTLN("thread({}) - potential collisions: {}", thread_id, pairs.size());
					PRINTLN("thread({}) - iterations: {}", thread_id, its);
					
				}
				// can clear old data now.
			});
	}
	// how check AABBs and generate pairs...

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
	
	timer.stop();
	timer_AABB += timer.get_time_us();
	// now I'm not sure what to do?
	// split into parallel jobs to calculate all contact points? Probably right?
	timer.start();
	const size_t pairs_per_thread = (merged_pairs.size() + num_threads - 1) / num_threads;
	std::vector<std::vector<ContactManifold>> vec_manifolds(num_threads);
	//std::vector<std::vector<CollisionResult>> vec_col_res(num_threads);
	for (size_t i = 0; i < num_threads; i++)
	{
		size_t thread_id = i;

		auto& manifolds = vec_manifolds[thread_id];
		auto& col_res = thread_col_res[thread_id];
		ctx.enqueue(
			[this, thread_id, pairs_per_thread, &manifolds, &merged_pairs, &col_res]()
			{
				
				size_t start_pair = pairs_per_thread * thread_id;
				size_t end_pair = std::min(start_pair + pairs_per_thread, merged_pairs.size());
				size_t size = std::max((end_pair - start_pair), (size_t)0);
				manifolds.reserve(1000);
				col_res.reserve(1000);
				for (size_t i = start_pair; i < end_pair; ++i)
				{
					bool any_physics = merged_pairs[i].layers_a.any_physics_collision(merged_pairs[i].layers_b);
						// these should all have rbs.
						ContactManifold manfild = OBB_collision_test(merged_pairs[i]); // need to check if it has RB or not
						if (manfild.has_collision)
						{
							if (merged_pairs[i].layers_a.is_collision_against(merged_pairs[i].layers_b.collision_layers))
							{
								// do something with collision info
								CollisionResult a;
								CollisionResult b;
								a.self_entity = merged_pairs[i].entity_a;
								b.self_entity = merged_pairs[i].entity_b;
								a.other_entity = merged_pairs[i].entity_b;
								b.other_entity = merged_pairs[i].entity_a;
								a.self_access_order = merged_pairs[i].access_order_a;
								b.self_access_order = merged_pairs[i].access_order_b;
								a.other_access_order = merged_pairs[i].access_order_b;
								b.other_access_order = merged_pairs[i].access_order_a;
								a.other_tag = merged_pairs[i].tag_b;
								b.other_tag = merged_pairs[i].tag_a;
								col_res.push_back(a);
								col_res.push_back(b);
							}
						}
						if (any_physics && merged_pairs[i].rb_a && merged_pairs[i].rb_b)
						{
							manifolds.push_back(manfild);
						}

				}
			});
	}
	ctx.gen_sync();
	ctx.entt_sync();
	timer.stop();
	timer_OBB += timer.get_time_us();
	float delta = ctx.get_delta_time();

	// iterate through attractors, create formula for attractor, and then make each object do the thing

	// now the landscapes have to be done.
	
	ctx.enqueue_parallel_each(WithRead<GravityStrengthComponent>{},
		WithWrite<RigidBodyComponent>{},
	[delta](const entt::entity entity, const GravityStrengthComponent& grav, RigidBodyComponent& rb)
		{
			if (!rb.asleep)
			{
				if (rb.linear_velocity.y > -30.f)
				{
					rb.linear_velocity -= glm::vec3(0, grav.gravity * delta, 0);
				}
				//rb.linear_velocity -= glm::vec3(0, 9.82f * delta, 0); // GRAVITY
			}
			
		}, 512);

	timer.start();
	std::vector<ContactManifold> merged_manifolds;
	for (auto& manifold : vec_manifolds)
	{
		merged_manifolds.insert(merged_manifolds.end(),
			std::make_move_iterator(manifold.begin()),
			std::make_move_iterator(manifold.end()));
	}
	merged_manifolds.insert(merged_manifolds.end(),
		std::make_move_iterator(landscape_manifolds.begin()),
		std::make_move_iterator(landscape_manifolds.end())
	);


	// now time for physics solver ... but I should have gathered rigidbodies by now... hm.. done that now
	// but before this, I should not have put those that lack rb into the manifolds.
	std::sort(merged_manifolds.begin(), merged_manifolds.end(), []
	(const ContactManifold& man_a, const ContactManifold& man_b)
		{
			if (man_a.access_order_a == man_b.access_order_a)
				return man_a.access_order_b > man_b.access_order_b;
			return man_a.access_order_a > man_b.access_order_a;
		});

	// now I need to "just" do physics resolve
	
	ctx.entt_sync();
	//now is good time to generate all the collision info

	//double dt = ctx.get_delta_time();
	size_t col_res_count = 0;
	std::vector<CollisionResult> merged_col_res;
	{
		
		for (const auto& col_res : thread_col_res)
			col_res_count += col_res.size();
		col_res_count += land_col_res.size();
		merged_col_res.reserve(col_res_count);
	}

	//ctx.enqueue([this, &thread_col_res, &merged_col_res]()
	//	{

		//});
	//and now write the info back to the entities... tricky...
	if (col_res_count > 0)
	{
		ctx.enqueue([&ctx, &thread_col_res, &merged_col_res, &land_col_res]()
			{
				for (auto& col_res : thread_col_res)
				{
					if (!thread_col_res.empty())
					{
						merged_col_res.insert(merged_col_res.end(),
							std::make_move_iterator(col_res.begin()),
							std::make_move_iterator(col_res.end()));
					}
				}
				merged_col_res.insert(merged_col_res.end(),
					std::make_move_iterator(land_col_res.begin()),
					std::make_move_iterator(land_col_res.end()));
				//std::sort(merged_col_res.begin(), merged_col_res.end(),
				//	[](const CollisionResult& a, const CollisionResult& b)
				//	{
				//		if (a.self_entity != b.self_entity)
				//			return a.self_entity < b.self_entity;
				//		return a.other_entity < b.other_entity;
				//	}
				//);
				std::sort(merged_col_res.begin(), merged_col_res.end(),
					[](const CollisionResult& a, const CollisionResult& b)
					{
						if (a.self_access_order != b.self_access_order)
							return a.self_access_order > b.self_access_order;
						return a.other_access_order > b.other_access_order;
					}
				);

				//for (const auto& col_res : merged_col_res)
				//{
				//	PRINTLN("pre-erase self: {}, other: {}, other tag: {}",
				//		static_cast<uint32_t>(col_res.self_entity),
				//		static_cast<uint32_t>(col_res.other_entity), col_res.other_tag);
				//}

				auto new_end = std::unique(
					merged_col_res.begin(),
					merged_col_res.end(),
					[](const CollisionResult& a, const CollisionResult& b)
					{
						return a.self_entity == b.self_entity &&
							a.other_entity == b.other_entity;
					}
				);
				merged_col_res.erase(new_end, merged_col_res.end());
				//PRINTLN("erased");

				entt::entity current_entity = merged_col_res[0].self_entity;
				size_t start = 0;
				size_t stop = 0;
				// this cound be multi threaded sorta
				// but tricky because you want to start on a new entity, and the work is kinda light
				for (const auto& col_res : merged_col_res)
				{
					//PRINTLN("entity: {}", static_cast<uint32_t>(col_res.self_entity));
					//PRINTLN("self: {}, other: {}, other tag: {}",
					//	static_cast<uint32_t>(col_res.self_entity),
					//	static_cast<uint32_t>(col_res.other_entity), col_res.other_tag);
					if (current_entity != col_res.self_entity)
					{
						// other entity ... so report to the old entity
						if (auto bbox = ctx.try_get<BoundingBoxComponent>(current_entity))
						{
							// report
							bbox->has_collision_info = true;
							bbox->result_start_index = start;
							bbox->result_end_index = stop;

						}
						current_entity = col_res.self_entity;
						start = stop;
					}
					++stop;
				}
				if (auto bbox = ctx.try_get<BoundingBoxComponent>(current_entity))
				{
					bbox->has_collision_info = true;
					bbox->result_start_index = start;
					bbox->result_end_index = stop;
				}
				ctx.get_system_storage().add_or_replace_object("CollisionResults", merged_col_res);
			}
		);
	}
	
	const int solver_iterations = 5; // 5-10 typical
	// now realize the rb changes into position and rotation
	float dt = (float)ctx.get_delta_time();
	for (int iteration = 0; iteration < solver_iterations; iteration++)
	{
		for (auto& manifold : merged_manifolds)
		{
			resolve_collision(manifold, landscape_rbs, dt);
		}
	}
	timer.stop();
	timer_resolve += timer.get_time_us();
	ctx.enqueue_parallel_each(
		WithWrite<RigidBodyComponent, TransformDynamicComponent>{},
		WithOut<LandscapeComponent>{},
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

			if (rb.is_dynamic_against(UINT8_MAX))
			{
				float lin_speed = glm::length(rb.linear_velocity);
				float ang_speed = glm::length(rb.angular_velocity);

				bool slow =
					lin_speed < sleep_linear_threshold &&
					ang_speed < sleep_angular_threshold;

				/*if (!slow && rb.asleep)
				{
					rb.sleep_strikes = 0;
					rb.asleep = false;
				}
				else if (slow && !rb.asleep)
				{
					++rb.sleep_strikes;

					if (rb.sleep_strikes > rb.max_sleep_strikes)
					{
						rb.asleep = true;
						rb.linear_velocity = glm::vec3(0);
						rb.angular_velocity = glm::vec3(0);
					}
				}*/

			}
		}, 512);
	ctx.gen_sync();


	if (count > 100)
	{
		PRINTLN("detected collisions {}", merged_manifolds.size());
		PRINTLN("Spatial: {}us", timer_spatial);
		PRINTLN("AABB: {}us", timer_AABB);
		PRINTLN("OBB: {}us", timer_OBB);
		PRINTLN("manifold: {}us", timer_manifold);
		PRINTLN("resolve: {}us", timer_resolve);
		timer_spatial = 0;
		timer_AABB = 0;
		timer_OBB = 0;
		timer_manifold = 0;
		timer_resolve = 0;

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

int PhysicsSystem::divide_int(float input, size_t divisor)
{
	
	if (input >= 0)
	{
		return (int)input / (int)divisor;
	}
	else
	{
		return -((int)(-input) / (int)divisor);
	}
}

ContactManifold PhysicsSystem::OBB_collision_test(const BroadPhasePair pair)
{
	ContactManifold manifold;
	manifold.entity_a = pair.entity_a;
	manifold.entity_b = pair.entity_b;
	manifold.has_collision = false;
	manifold.penetration_depth = FLT_MAX;
	manifold.landscape_a = pair.landscape_a;
	manifold.landscape_b = pair.landscape_b;

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
		if (manifold.num_contact_points >= 7)
		{
			break;
		}
		if (is_point_in_OBB(corner, obb_b, epsilon))
		{

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
		if (manifold.num_contact_points >= 7)
		{
			break;
		}
		if (is_point_in_OBB(corner, obb_a, epsilon))
		{

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
				for (size_t k = j; k + 1 < manifold.num_contact_points; ++k)
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

void PhysicsSystem::resolve_collision(ContactManifold& manifold, std::vector<RigidBodyComponent>& landscape_rbs, float dt)
{
	// ==========================================
 // SAFETY CHECK
 // ==========================================

	RigidBodyComponent* rb_a = manifold.rb_a;
	RigidBodyComponent* rb_b = manifold.rb_b;
	if (manifold.landscape_a) rb_a = &landscape_rbs[manifold.landscape_index];
	if (manifold.landscape_b) rb_b = &landscape_rbs[manifold.landscape_index];
	if (!rb_a || !rb_b) return;

	// fix for static objects
	if (manifold.rb_b->is_static_against(manifold.rb_a->static_layers) && 
		!(manifold.rb_a->is_dynamic_against(manifold.rb_b->dynamic_layers) || 
			manifold.rb_b->is_dynamic_against(manifold.rb_a->static_layers)||
			manifold.rb_a->is_dynamic_against(manifold.rb_b->static_layers))) return; // I think these are the same though
	bool dynamic_a = manifold.rb_a->is_dynamic_against(manifold.rb_b->dynamic_layers) || manifold.rb_a->is_dynamic_against(manifold.rb_b->static_layers);
	bool dynamic_b = manifold.rb_b->is_dynamic_against(manifold.rb_a->dynamic_layers) || manifold.rb_b->is_dynamic_against(manifold.rb_a->static_layers);
	

	float inverse_mass_a = dynamic_a ? rb_a->inverse_mass : 0.0f;
	float inverse_mass_b = dynamic_b ? rb_b->inverse_mass : 0.0f;

	glm::mat3 inverse_inertia_tensor_a = (float)dynamic_a * manifold.rb_a->inverse_inertia_tensor_local;
	glm::mat3 inverse_inertia_tensor_b = (float)dynamic_b * manifold.rb_b->inverse_inertia_tensor_local;

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
	const float baumgarte_factor = 0.1f;     // Correct 20% of penetration per iteration

	if (manifold.penetration_depth > penetration_slop)
	{
		// How much penetration do we need to fix?
		float excess_penetration = manifold.penetration_depth - penetration_slop;

		// Only correct a fraction of it (stability)
		float correction_amount = excess_penetration * baumgarte_factor;

		

		// Total inverse mass (if both movable, they share the work)
		float total_inv_mass = inverse_mass_a + inverse_mass_b; //rb_a->inverse_mass + rb_b->inverse_mass;

		if (total_inv_mass > 0.0f) // At least one object is movable
		{
			// The correction vector (how much to move apart)
			glm::vec3 correction = manifold.contact_normal * correction_amount;

			// Move each object proportional to its inverse mass
			// Lighter object (higher inverse_mass) moves MORE
			// Heavier object (lower inverse_mass) moves LESS
			// Immovable object (inverse_mass = 0) doesn't move at all
			rb_a->position -= correction * (inverse_mass_a / total_inv_mass);
			rb_b->position += correction * (inverse_mass_b / total_inv_mass);
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
			inverse_inertia_tensor_a *
			glm::transpose(rotation_matrix_a);

		glm::mat3 inv_inertia_world_b =
			rotation_matrix_b *
			inverse_inertia_tensor_b *
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
			inverse_mass_a +
			inverse_mass_b +
			angular_term;

		// The impulse magnitude (scalar)
		// Formula: j = -(1 + e) * v_rel / effective_mass
		float impulse_magnitude =
			-(1.0f + restitution) * velocity_along_normal / effective_inv_mass;

		// Convert to vector (direction = contact normal)
		glm::vec3 normal_impulse = impulse_magnitude * manifold.contact_normal;

		// TAMING THE COLLISIONS
		//float contact_weight = 1.0f / float(manifold.num_contact_points);
		//normal_impulse *= contact_weight;

		// --------------------------------------
		// 2.5: Apply Normal Impulse
		// --------------------------------------
		// An impulse is a change in momentum: Δp = F * Δt
		// Change in velocity: Δv = Δp / m = impulse * inverse_mass

		// LINEAR: Apply impulse to linear velocity
		rb_a->linear_velocity -= normal_impulse * inverse_mass_a;
		rb_b->linear_velocity += normal_impulse * inverse_mass_b;

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
				inverse_mass_a + inverse_mass_b +
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
			rb_a->linear_velocity -= friction_impulse * inverse_mass_a;
			rb_b->linear_velocity += friction_impulse * inverse_mass_b;

			rb_a->angular_velocity -= inv_inertia_world_a * glm::cross(r_a, friction_impulse);
			rb_b->angular_velocity += inv_inertia_world_b * glm::cross(r_b, friction_impulse);
		}

		//if (!rb_a->is_dynamic_against(UINT8_MAX))
		//{
		//	float lin_speed = glm::length(rb_a->linear_velocity);
		//	float ang_speed = glm::length(rb_a->angular_velocity);

		//	bool slow =
		//		lin_speed < sleep_linear_threshold &&
		//		ang_speed < sleep_angular_threshold;

		//	if (slow)
		//	{
		//		++rb_a->sleep_strikes;

		//		if (rb_a->sleep_strikes > rb_a->max_sleep_strikes)
		//		{
		//			rb_a->asleep = true;
		//			rb_a->linear_velocity = glm::vec3(0);
		//			rb_a->angular_velocity = glm::vec3(0);
		//		}
		//	}
		//	else
		//	{
		//		rb_a->sleep_strikes = 0.0f;
		//		rb_a->asleep = false;
		//	}
		//}

		/*if (rb_a->asleep)
		{
			float lin_speed = glm::length(rb_a->linear_velocity);
			float ang_speed = glm::length(rb_a->angular_velocity);

			if (lin_speed < sleep_linear_threshold &&
				ang_speed < sleep_angular_threshold)
			{
				rb_a->asleep = true;
			}
			else
			{
				rb_a->asleep = false;
				rb_a->sleep_strikes = 0;
			}
		}
		if (rb_b->asleep)
		{
			float lin_speed = glm::length(rb_b->linear_velocity);
			float ang_speed = glm::length(rb_b->angular_velocity);

			if (lin_speed < sleep_linear_threshold &&
				ang_speed < sleep_angular_threshold)
			{
				rb_b->asleep = true;
			}
			else
			{
				rb_a->asleep = false;
				rb_a->sleep_strikes = 0;
			}
		}*/
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
				landscape.map.max_height = landscape.max_height;
				auto& system_storage = ctx.get_system_storage();
				if (generate_heightmap(landscape.height_map_path.string, landscape.map))
				{
					landscape.has_loaded_height_map = true;
					system_storage.add_or_replace_object(landscape.height_map_path.string, landscape.map);

				}
			}
		});
	return false;
}

bool HeightMapLoadSystem::generate_heightmap(const std::string& path, HeightMap& map)
{

	int width, height, channels;
	
	//stbi_set_flip_vertically_on_load(true);
	unsigned char* height_data = stbi_load(path.c_str(), &width, &height, &channels, 0);
	if (!height_data || width <= 0 || height <= 0)
	{ 
		PRINTLN("BAD DATA");
		return false;
	}
	map.height_map.clear();
	map.height_map.reserve((size_t)width * (size_t)height);
	map.height = height;
	map.width = width;

	float max_height = map.max_height;
	for (int v = height - 1; v >= 0; v--)  // ← Start from bottom, go to top
	{
		for (int u = 0; u < width; u++)
		{
			int index = (v * width + u) * channels;
			unsigned char red = height_data[index];
			float h = ((float)red / 255.f) * max_height;
			map.height_map.push_back(h);
		}
	}
	stbi_image_free(height_data);
	return true;
}

bool UIDrawGathering::execute(SystemsContext& ctx)
{
	auto& sys_strg = ctx.get_system_storage();

	auto menu_open = sys_strg.get_object<bool>("MenuOpen");
	if (menu_open)
	{
		bool is_open = false;
		menu_open->read(
			[this, &is_open](const bool& open)
			{
				is_open = open;
			});
		if (!is_open)
			return false;
	}
	else return false;


	std::vector<UIPreRenderRequest> new_rects;
	sys_strg.add_or_replace_object("UIRects", new_rects);
	
	std::vector<UIPreRenderRequest> new_texts;
	sys_strg.add_or_replace_object("UIWords", new_texts);

	std::vector<hashed_string_64> ui_mats;
	sys_strg.add_or_replace_object("UIMats", ui_mats);

	auto engine_context = ctx.get_engine_context();
	ctx.enqueue([&ctx, engine_context]()
		{
			uint64_t invalid = hashed_string_64("").hash;
			auto& sys_strg = ctx.get_system_storage();
			SystemsStorageObject<std::vector<hashed_string_64>>* UIMats
				= sys_strg.get_object<std::vector<hashed_string_64>>("UIMats");
			UIMats->write([&ctx, &invalid](std::vector<hashed_string_64>& mats)
				{
					mats.clear();
					ctx.each(WithRead<const UIMaterialComponent>{},
						[&invalid, &mats]
						(const entt::entity entity, const UIMaterialComponent mat)
						{
							if (mat.draw && mat.material.hash != invalid)
							{
								mats.push_back(mat.material);
							}
						});
					if (!mats.empty())
					{
						std::sort(mats.begin(), mats.end());
						auto last = unique(mats.begin(), mats.end());
						mats.erase(last, mats.end());
					}

				});
		}
	);

	// from now on I don't care about stupid naming, vars will just be letters
	ctx.enqueue([&ctx]()
		{
			auto& sys_strg = ctx.get_system_storage();
			SystemsStorageObject<std::vector<UIPreRenderRequest>>* UIRects
				= sys_strg.get_object<std::vector<UIPreRenderRequest>>("UIRects");
			UIRects->write([&ctx](std::vector<UIPreRenderRequest>& ui_rects)
				{
					uint64_t invalid = hashed_string_64("").hash;
					ui_rects.clear();
					ctx.each(WithRead<const UIRectComponent, const UIMaterialComponent>{},
						[&ui_rects, &invalid](const entt::entity entity, const UIRectComponent& r, const UIMaterialComponent& m)
						{
							if (m.draw && m.material.hash != invalid)
							{
								// just want to add necessary data...
								UIPreRenderRequest rect;
								rect.color = r.color;
								rect.extents = r.size;
								rect.position = r.position;
								rect.order = r.order;
								rect.mat_hash = m.material.hash;
								rect.uv_offset = r.uv_offset;
								ui_rects.push_back(rect);
							}

						}
					);
					if (!ui_rects.empty())
					{
						std::sort(ui_rects.begin(), ui_rects.end(),
							[](const auto& a, const auto& b)
							{
								return a.order < b.order;
							});
					}
				});

		}
	);
	ctx.enqueue([this, &ctx]()
		{
			auto& sys_strg = ctx.get_system_storage();
			SystemsStorageObject<std::vector<UIPreRenderRequest>>* UITexts
				= sys_strg.get_object<std::vector<UIPreRenderRequest>>("UIWords");
			UITexts->write([this, &ctx](std::vector<UIPreRenderRequest>& ui_words)
				{
					uint64_t invalid = hashed_string_64("").hash;
					ui_words.clear();
					ctx.each(WithRead<const UITextComponent, const UIMaterialComponent>{},
						[this, &ui_words, &invalid](const entt::entity, const UITextComponent& t, const UIMaterialComponent& m)
						{
							if (m.draw && m.material.hash != invalid)
							{
								float fs = t.font_size;
								int max_row = static_cast<int>(t.size.x / fs);
								int max_col = static_cast<int>(t.size.y / fs);

								int row = 0;
								int col = 0;
								if (max_row < 1 && max_col < 1 || t.text.length() < 1) return;
								for (const auto& word : t.text)
								{
									UIPreRenderRequest w;
									w.color = t.color;
									w.extents = glm::vec2(fs);

									if (word == '\n' || row > max_row)
									{
										++col;
										row = 0;
									}
									if (col > max_col) return;

									w.uv_offset = char_to_offset(word);
									w.order = t.order;

									// ah crap, still have to do the ndc offset
									w.position = (glm::vec2(1.33f * row * fs, -col * fs * 1.75f)) - (-t.position);
									// probably right, I pray
									w.mat_hash = m.material.hash;
									ui_words.push_back(w);
									++row;
								}
							}
						});
					if (!ui_words.empty())
					{
						std::sort(ui_words.begin(), ui_words.end(),
							[](const auto& a, const auto& b)
							{
								return a.order < b.order;
							});
					}
				});
		}
	);

	return false;
}

glm::vec2 UIDrawGathering::char_to_offset(const char& c)
{
	int i = static_cast<int>(c - 32);
	if (i < 0 || i > 95) // well can't be over 95...
	{
		// return special error character
		return glm::vec2(0.9f, 0.9f);
	}

	// we'll do a 10x10 atlas I guess
	int col = i % 10;
	int row = i / 10;

	float u = 0.1f * col;
	float v = 1.0f - 0.1f * (row + 1);
	return glm::vec2(u,v);
}

bool MenuSystem::execute(SystemsContext& ctx)
{
	// should check if a key has been pressed, then toggle the menu mode
	// if menu mode, draw a UIRect I guess
	// move UIRect by delta
	// 
	// register the position into system context storage
	// or just straight up iterate through all of the UIs and check em.
	bool synced = false;
	auto& input_manager = InputManager::instance();
	if (input_manager.is_key_pressed(EKey::Escape) && !just_changed)
	{
		menu_open = !menu_open;
		just_changed = true;
		if (menu_open)
		{
			mouse_position = glm::vec2(0);
		}
	}
	else if (input_manager.is_key_released(EKey::Escape) && just_changed)
	{
		just_changed = false;
	}

	if (InputManager::instance().is_key_pressed(EKey::LeftMouseButton) && just_pressed)
	{
		can_press = false;
		just_pressed = false;
	}
	else if (InputManager::instance().is_key_released(EKey::LeftMouseButton) && just_pressed)
	{
		can_press = true;
		just_pressed = true;
	}

	auto& strg = ctx.get_system_storage();
	strg.add_or_replace_object("MenuOpen", menu_open);
	ctx.enqueue(
		[this, &ctx]() 
		{
			if (menu_open)
			{
				auto mouse_delta = glm::vec2(0, 0);
				InputManager::instance().get_mouse_delta(mouse_delta.x, mouse_delta.y);
				mouse_delta.y *= -1.f;
				mouse_position += mouse_delta * 0.005f;
				mouse_position.x = std::max(-1.f, std::min(1.f, mouse_position.x));
				mouse_position.y = std::max(-1.f, std::min(1.f, mouse_position.y));

				ctx.each(WithRead<CursorComponent>{},
					WithWrite<UIRectComponent>{},
					[this]
					(const entt::entity entity, CursorComponent& cursor, UIRectComponent& rect)
					{
						// add sensitivity to the cursorcomponent ... or maybe not?
						rect.position = mouse_position;
					}
				);
				uint32_t state_layer = 0;
				entt::entity menu_entity = entt::null;
				ctx.each(WithRead<UIMenuControllerComponent>{},
					[&state_layer, &menu_entity](const entt::entity entity, const UIMenuControllerComponent& menu)
					{
						state_layer = menu.active_state_layers;
						menu_entity = entity;

					});
				bool valid_press = can_press;
			
				// technically race condition because we access outside entity, 
				// but two entities should never modify it at the same time
				ctx.enqueue_parallel_each(
					WithRead<UISelectableComponent, UIDependencyComponent, UITypeComponent>{},
					WithWrite<UIMenuItemStatusComponent, UIMaterialComponent>{},
					WithRef<UIColorChangeComponent, UIRectComponent, UITextComponent, ScenePathComponent>{},
					[this, &ctx, state_layer, menu_entity, valid_press]
					(
						const entt::entity entity,
						const UISelectableComponent sel,
						const UIDependencyComponent dep,
						const UITypeComponent type,
						UIMenuItemStatusComponent& stat,
						UIMaterialComponent& mat
						)
					{
						if (!dep.matches(state_layer))
						{
							mat.draw = false;
							return;
						}
						mat.draw = true;

						if (auto rect_ptr = ctx.try_get<UIRectComponent>(entity))
						{
							auto& rect = *rect_ptr;
							bool inside =
								mouse_position.x > rect.position.x &&
								mouse_position.x < rect.position.x + rect.size.x &&
								mouse_position.y < rect.position.y &&
								mouse_position.y > rect.position.y - rect.size.y;
							if (inside)
							{
								stat.hovered = true;
								if (auto col = ctx.try_get<UIColorChangeComponent>(entity))
								{
									rect.color = col->active;
								}
								if (InputManager::instance().is_key_pressed(EKey::LeftMouseButton))
								{
									stat.pressed = true;
								}
								else stat.pressed = false;
							}
							else
							{
								if (auto col = ctx.try_get<UIColorChangeComponent>(entity))
								{
									rect.color = col->inactive;
								}
								stat.hovered = false;
								stat.pressed = false;
							}
						}
						else if (auto txt = ctx.try_get<UITextComponent>(entity))
						{
							auto& text = *txt;
							bool inside =
								mouse_position.x > text.position.x &&
								mouse_position.x < text.position.x + text.size.x &&
								mouse_position.y < text.position.y &&
								mouse_position.y > text.position.y - text.size.y;

							if (inside)
							{
								stat.hovered = true;
								if (auto col = ctx.try_get<UIColorChangeComponent>(entity))
								{
									text.color = col->active;
								}
								if (InputManager::instance().is_key_pressed(EKey::LeftMouseButton) && valid_press)
								{
									stat.pressed = true;

									if (auto menu_cont = ctx.try_get<UIMenuControllerComponent>(menu_entity))
									{
										auto& new_layers = menu_cont->active_state_layers;
										switch (type.type)
										{
											//I think go back to main menu
										case 0:
											// I think?
											new_layers = 0;
											new_layers |= UIMainMenu;
											break;
											// now I think display layers
										case 1:
											new_layers = 0;
											new_layers |= UILevels;
											break;
										case 2:
											new_layers = 0;
											new_layers |= UIHighScores;
											break;
										case 5:
											if (auto path = ctx.try_get<ScenePathComponent>(entity))
											{
												if (!path->path.empty())
												{
													LoadSceneCommand scene_command;
													scene_command.load = true;
													scene_command.path = path->path;
													ctx.get_system_storage().add_or_replace_object("LoadSceneCommand", scene_command);
												}
											}
											break;
										case 10:
											ctx.get_system_storage().add_or_replace_object("ExitGame", true);
											break;

										default:
											break;
										}
									}
									// ok now here we do this thing...
									//LoadSceneCommand scene_command;
									//scene_command.load = true;
									//scene_command.path = "./assets/scenes/menutest.scn";
									//ctx.get_system_storage().add_or_replace_object("LoadSceneCommand", scene_command);
								}
								else stat.pressed = false;
							}
							else
							{
								if (auto col = ctx.try_get<UIColorChangeComponent>(entity))
								{
									text.color = col->inactive;
								}
								stat.hovered = false;
								stat.pressed = false;
							}
						}

					}
				);
			}
		});

	
	return synced;
}

bool ReadCollisionResultSystem::execute(SystemsContext& ctx)
{
	col_res = ctx.get_system_storage().get_object<std::vector<CollisionResult>>("CollisionResults");
	if (col_res)
	{
		//PRINTLN("new frame");
		col_res->copy(collisions);
		ctx.enqueue_each(WithRead<BoundingBoxComponent>{},
			[this, &ctx](const entt::entity entity, const BoundingBoxComponent& bbox)
			{
				if (bbox.has_collision_info)
				{
					for (size_t i = bbox.result_start_index; i < bbox.result_end_index; i++)
					{
						const auto& res = collisions[i];
						PRINTLN("true self: {}, self: {}, other: {}, other tag: {}",
							static_cast<uint32_t>(entity), static_cast<uint32_t>(res.self_entity),
							static_cast<uint32_t>(res.other_entity), res.other_tag);
					}
				}
			}
		);
	}
	return false;
}

bool RenderRequestsSystem::execute(SystemsContext& ctx)
{

	ctx.enqueue(
		[this, &ctx]()
		{
			auto& sys_strg = ctx.get_system_storage();

			auto menu_open = sys_strg.get_object<bool>("MenuOpen");
			if (menu_open)
			{
				bool is_open = false;
				menu_open->read(
					[this, &is_open](const bool& open)
					{
						is_open = open;
					});
				if (!is_open)
					return false;
			}
			else return false;


			std::vector<UIPreRenderRequest> new_rects;
			sys_strg.add_or_replace_object("UIRects", new_rects);

			std::vector<UIPreRenderRequest> new_texts;
			sys_strg.add_or_replace_object("UIWords", new_texts);

			std::vector<hashed_string_64> ui_mats;
			sys_strg.add_or_replace_object("UIMats", ui_mats);

			auto engine_context = ctx.get_engine_context();
			ctx.enqueue([&ctx, engine_context]()
				{
					uint64_t invalid = hashed_string_64("").hash;
					auto& sys_strg = ctx.get_system_storage();
					SystemsStorageObject<std::vector<hashed_string_64>>* UIMats
						= sys_strg.get_object<std::vector<hashed_string_64>>("UIMats");
					UIMats->write([&ctx, &invalid](std::vector<hashed_string_64>& mats)
						{
							mats.clear();
							ctx.each(WithRead<const UIMaterialComponent>{},
								[&invalid, &mats]
								(const entt::entity entity, const UIMaterialComponent mat)
								{
									if (mat.draw && mat.material.hash != invalid)
									{
										mats.push_back(mat.material);
									}
								});
							if (!mats.empty())
							{
								std::sort(mats.begin(), mats.end());
								auto last = unique(mats.begin(), mats.end());
								mats.erase(last, mats.end());
							}

						});
				}
			);

			// from now on I don't care about stupid naming, vars will just be letters
			ctx.enqueue([&ctx]()
				{
					auto& sys_strg = ctx.get_system_storage();
					SystemsStorageObject<std::vector<UIPreRenderRequest>>* UIRects
						= sys_strg.get_object<std::vector<UIPreRenderRequest>>("UIRects");
					UIRects->write([&ctx](std::vector<UIPreRenderRequest>& ui_rects)
						{
							uint64_t invalid = hashed_string_64("").hash;
							ui_rects.clear();
							ctx.each(WithRead<const UIRectComponent, const UIMaterialComponent>{},
								[&ui_rects, &invalid](const entt::entity entity, const UIRectComponent& r, const UIMaterialComponent& m)
								{
									if (m.draw && m.material.hash != invalid)
									{
										// just want to add necessary data...
										UIPreRenderRequest rect;
										rect.color = r.color;
										rect.extents = r.size;
										rect.position = r.position;
										rect.order = r.order;
										rect.mat_hash = m.material.hash;
										rect.uv_offset = r.uv_offset;
										ui_rects.push_back(rect);
									}

								}
							);
							if (!ui_rects.empty())
							{
								std::sort(ui_rects.begin(), ui_rects.end(),
									[](const auto& a, const auto& b)
									{
										return a.order < b.order;
									});
							}
						});

				}
			);
			ctx.enqueue([this, &ctx]()
				{
					auto& sys_strg = ctx.get_system_storage();
					SystemsStorageObject<std::vector<UIPreRenderRequest>>* UITexts
						= sys_strg.get_object<std::vector<UIPreRenderRequest>>("UIWords");
					UITexts->write([this, &ctx](std::vector<UIPreRenderRequest>& ui_words)
						{
							uint64_t invalid = hashed_string_64("").hash;
							ui_words.clear();
							ctx.each(WithRead<const UITextComponent, const UIMaterialComponent>{},
								[this, &ui_words, &invalid](const entt::entity, const UITextComponent& t, const UIMaterialComponent& m)
								{
									if (m.draw && m.material.hash != invalid)
									{
										float fs = t.font_size;
										int max_row = static_cast<int>(t.size.x / fs);
										int max_col = static_cast<int>(t.size.y / fs);

										int row = 0;
										int col = 0;
										if (max_row < 1 && max_col < 1 || t.text.length() < 1) return;
										for (const auto& word : t.text)
										{
											UIPreRenderRequest w;
											w.color = t.color;
											w.extents = glm::vec2(fs);

											if (word == '\n' || row > max_row)
											{
												++col;
												row = 0;
											}
											if (col > max_col) return;

											w.uv_offset = char_to_offset(word);
											w.order = t.order;

											// ah crap, still have to do the ndc offset
											w.position = (glm::vec2(1.33f * row * fs, -col * fs * 1.75f)) - (-t.position);
											// probably right, I pray
											w.mat_hash = m.material.hash;
											ui_words.push_back(w);
											++row;
										}
									}
								});
							if (!ui_words.empty())
							{
								std::sort(ui_words.begin(), ui_words.end(),
									[](const auto& a, const auto& b)
									{
										return a.order < b.order;
									});
							}
						});
				}
			);
		});

	size_t num_threads = ctx.num_threads();
	vec_pre_render_reqs.resize(num_threads);
	for (auto& vec : vec_pre_render_reqs)
	{
		vec.reserve(prev_size);
	}
	ctx.enqueue_parallel_data_each(
	WithRead<RenderComponent, TransformDynamicComponent>{},
	[](
		std::vector<PreRenderReq>& pre_render_reqs, 
		const entt::entity entity, 
		const RenderComponent& render, 
		const TransformDynamicComponent& transform
		)
	{
			pre_render_reqs.emplace_back(transform.model_matrix, render.mesh.hash,
				render.material.hash, render.instance, render.mipmap);
	}, vec_pre_render_reqs, num_threads);
	ctx.enqueue(WithWrite<RenderComponent>{},
		[this, &ctx]()
		{
				size_t total_size = 0;
				prev_size = 0;
				for (const auto& vec : vec_pre_render_reqs)
				{
					prev_size = std::max(prev_size, vec.size());
					total_size += vec.size();
				}
				std::vector<PreRenderReq> pre_render_reqs;
				pre_render_reqs.reserve(total_size);
				for (auto& vec : vec_pre_render_reqs)
				{
					pre_render_reqs.insert(pre_render_reqs.end(),
						std::make_move_iterator(vec.begin()),
						std::make_move_iterator(vec.end()));
				}
				ctx.get_system_storage().add_or_replace_object("PreRenderReqs", pre_render_reqs);
				vec_pre_render_reqs.clear();
		});

	return false;
}

glm::vec2 RenderRequestsSystem::char_to_offset(const char& c)
{
	int i = static_cast<int>(c - 32);
	if (i < 0 || i > 95) // well can't be over 95...
	{
		// return special error character
		return glm::vec2(0.9f, 0.9f);
	}

	// we'll do a 10x10 atlas I guess
	int col = i % 10;
	int row = i / 10;

	float u = 0.1f * col;
	float v = 1.0f - 0.1f * (row + 1);
	return glm::vec2(u, v);
}

bool PlayerMovementSystem::execute(SystemsContext& ctx)
{
	float dt = (float)ctx.get_delta_time();
	ctx.each(
		WithRead<PlayerTagComponent, MovementStatsComponent>{},
		WithWrite<RigidBodyComponent>{},
		[dt](const entt::entity entity, const PlayerTagComponent& tag, const MovementStatsComponent& mov, RigidBodyComponent& rb)
		{

			rb.linear_velocity.x -= (rb.linear_velocity.x * mov.move_damp * dt);
			rb.linear_velocity.z -= (rb.linear_velocity.z * mov.move_damp * dt);

			if (InputManager::instance().is_key_pressed(EKey::Space) && mov.time_since_ground < 0.1f)
			{
				rb.linear_velocity.y += mov.jump_force;
			}
			if (InputManager::instance().is_key_held(EKey::S))
			{
				rb.linear_velocity.z = std::min(rb.linear_velocity.z + mov.move_force * dt, mov.max_move_speed);
			}
			if (InputManager::instance().is_key_held(EKey::W))
			{
				rb.linear_velocity.z = std::max(rb.linear_velocity.z - mov.move_force * dt, -mov.max_move_speed);
			}
			if (InputManager::instance().is_key_held(EKey::D))
			{
				rb.linear_velocity.x = std::min(rb.linear_velocity.x + mov.move_force * dt, mov.max_move_speed);
			}
			if (InputManager::instance().is_key_held(EKey::A))
			{
				rb.linear_velocity.x = std::max(rb.linear_velocity.x - mov.move_force * dt, -mov.max_move_speed);
			}
			
		});
	return false;
}

bool GroundingSystem::execute(SystemsContext& ctx)
{
	
	ctx.enqueue(
		[this, &ctx]()
		{
			col_res.clear();
			float dt = (float)ctx.get_delta_time();
			if (auto res = ctx.get_system_storage().get_object<std::vector<CollisionResult>>("CollisionResults"))
			{
				res->copy(col_res); // not great to copy, but this would be a parallel job, so kinda have to
			}
			if (!col_res.empty())
			{
				size_t size = col_res.size();
				ctx.enqueue_parallel_each(
					WithRead<BoundingBoxComponent>{},
					WithWrite<MovementStatsComponent>{},
					[this, size, dt](const entt::entity entity, const BoundingBoxComponent& bbox, MovementStatsComponent& mov)
					{
						mov.time_since_ground += dt;
						if (bbox.has_collision_info)
						{
							for (size_t i = bbox.result_start_index; i < bbox.result_end_index; ++i)
							{
								if (i < size && col_res[i].other_tag == 10)
								{
									mov.time_since_ground = 0.f;
									break;
								}
							}
						}
					});
			}
		});


	return false;
}

bool ButtonSystem::execute(SystemsContext& ctx)
{
	ctx.enqueue(WithWrite<EntityCommandBufferComponent>{},
		[this, &ctx]()
		{
			if (auto res = ctx.get_system_storage().get_object<std::vector<CollisionResult>>("CollisionResults"))
			{
				res->read([&ctx](const std::vector<CollisionResult>& cols)
					{
						if (!cols.empty())
						{
							ctx.each(WithRead<BoundingBoxComponent, ActivationReferenceComponent>{},
								WithWrite<ButtonObjectTag>{},
								[&ctx, &cols](const entt::entity entity, const BoundingBoxComponent& bbox, const ActivationReferenceComponent& a, ButtonObjectTag& bt)
								{
									if (!bt.active && bbox.has_collision_info)
									{
										for (size_t i = bbox.result_start_index; i < bbox.result_end_index; i++)
										{
											if (i < cols.size() && cols[i].other_tag == 1)
											{
												if (auto ent = ctx.try_get<EntityComponent>(a.reference.entity))
												{
													bt.active = true;
													if (auto ecb = ctx.get_ecb("end_frame_ecb"))
													{
														ecb->destroy_entity(a.reference.entity);
													}
													else
													{
														PRINTLN("ecb didn't exist");
													}
													break;
												}
											}
										}
									}
								});
						}
						
					});
			}
		});

	return false;
}

bool CollectionSystem::execute(SystemsContext& ctx)
{
	// will need a "point counter" component or something, for the coin counter manager
	// or just store the value in a systemstorageobject
	
	// get all collection things
	// destroy self if you found collection
	// but also +1 on the shared object

	ctx.enqueue(
		WithWrite<EntityCommandBufferComponent>{},
		[this, &ctx]()
		{
			size_t coins_collected = 0;
			if (auto res = ctx.get_system_storage().get_object<std::vector<CollisionResult>>("CollisionResults"))
			{
				res->read(
					[&ctx, &coins_collected](const std::vector<CollisionResult>& cols)
					{
						if (!cols.empty())
						{
							
							ctx.each(WithRead<BoundingBoxComponent>{},
								WithWrite<CollectionObjectTag>{},
								[&ctx, &cols, &coins_collected](const entt::entity entity, const BoundingBoxComponent& bbox, CollectionObjectTag& coin)
								{
									if (bbox.has_collision_info)
									{
										for (size_t i = bbox.result_start_index; i < bbox.result_end_index; i++)
										{
											if (i < cols.size() && cols[i].other_tag == 1)
											{
												++coins_collected;
												if (auto ecb = ctx.get_ecb("end_frame_ecb"))
												{
													ecb->destroy_entity(entity);
												}
												else
												{
													PRINTLN("ecb didn't exist");
												}
												break;
											}
										}
									}
								});
						}
					});
				if (coins_collected > 0)
				{
					if (auto coins = ctx.get_system_storage().get_object<size_t>("CollectedCoins"))
					{
						coins->write([&coins_collected](size_t& tot_coins)
							{
								tot_coins += coins_collected;
							});
					}
					else
					{
						ctx.get_system_storage().add_or_replace_object("CollectedCoins", coins_collected);
					}
				}
				}

		});

	//ctx.enqueue_each(WithRead<>);
	return false;
}

bool ECBEndSystem::execute(SystemsContext& ctx)
{
	ctx.sync();
	ctx.execute_ecb("end_frame_ecb");
	return false;
}
