#include <engine/game/systems.h>
#include <engine/game/components.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <engine/utilities/macros.h>

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

bool AABBWriteSpatialPartitionSystem::execute(SystemsContext& ctx)
{
	using SpatialPartition = std::unordered_map<CellCoordinates, std::vector<entt::entity>>;
	size_t num_threads = ctx.num_threads();
	//ctx.enqueue([&ctx]()
	//	{
		//auto vectored_spatial_map = std::vector<std::unordered_map<CellCoordinates, std::vector<entt::entity>>>();
	vectored_spatial_map.clear();
	vectored_spatial_map.resize(num_threads);

	ctx.enqueue_parallel_data_each(
		WithRead<TransformDynamicComponent, BoundingBoxComponent>{},
		[this]( SpatialPartition& spatial_partition,
			const entt::entity entity, 
			const TransformDynamicComponent& transform,
			const BoundingBoxComponent& bbox )
		{
			const glm::mat4& model_matrix = transform.model_matrix;

			glm::vec3 half_extents = bbox.box_extents * 0.5f;
			glm::mat3 scaled_rotation = glm::mat3(model_matrix);

			glm::vec3 offset_rot_scaled_box = glm::vec3(model_matrix * glm::vec4(bbox.offset, 1.f));

			glm::mat3 max_scaled_rotation = glm::mat3(
				glm::abs(scaled_rotation[0]),
				glm::abs(scaled_rotation[1]),
				glm::abs(scaled_rotation[2])
			);

			glm::vec3 world_half_extents = max_scaled_rotation * half_extents;

			glm::vec3 aabb_min = offset_rot_scaled_box - world_half_extents;
			glm::vec3 aabb_max = offset_rot_scaled_box + world_half_extents;
			
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

			for (int x = min_cell.x; x <= max_cell.x; ++x)
				for (int y = min_cell.y; y <= max_cell.y; ++y)
					for (int z = min_cell.z; z <= max_cell.z; ++z)
						spatial_partition[{x, y, z}].push_back(entity);
			

		}, vectored_spatial_map, num_threads);
	ctx.entt_sync(); // could push vectored_spatial_map into the storage to retrrieve this->vectored_spatial_map later to combine

		// I could send in an extra lambda in the above, as an "after completion" to insert in parallel in chunks
			
		//ctx.enqueue([&vectored_spatial_map = this->vectored_spatial_map, &ctx]() // technically not "safe"
		//	{
	SpatialPartition data;
	for (auto& spatial_map : vectored_spatial_map)
		for (auto& [coords, entities] : spatial_map)
			data[coords].insert(data[coords].end(), entities.begin(), entities.end());

	auto& system_storage = ctx.get_system_storage();
	system_storage.add_or_replace_object("AABBSpatialPartition", std::move(data));
	system_storage.add_or_replace_object("SpatialCellSize", cell_size);
	//if (auto storage_object = 
	//	system_storage.get_object<SpatialPartition>("AABBSpatialPartition"))
	//{
	//	storage_object->write([this]
	//	(SpatialPartition& data)
	//		{
	//			data.clear();
	//			for (auto& spatial_map : vectored_spatial_map)
	//				for (auto& [coords, entities] : spatial_map)
	//					data[coords].insert(data[coords].end(), entities.begin(), entities.end());
	//			PRINTLN("updated spatialParition");
	//		});
	//}
	//else
	//{

	//}
			//});

		//system_storage.add_or_replace_object("PhysicsHashMap", std::move(hash_map));
	//});


	return false;
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
