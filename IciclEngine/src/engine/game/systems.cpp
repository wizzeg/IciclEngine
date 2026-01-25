#include <engine/game/systems.h>
#include <engine/game/components.h>
#include <glm/glm.hpp>

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
