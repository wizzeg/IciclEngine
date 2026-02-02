#include <engine/core/game_thread.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
// #include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <engine/editor/scene_object.h>
#include <engine/renderer/render_info.h>
#include <glad/glad.h>
#include <KHR/khrplatform.h>
#include <glfw/glfw3.h>
#include <engine/utilities/utilities.h>
#include <engine/game/systems.h>


void GameThread::execute()
{
	uint64_t prev_test = 0;
	int runs = 0;

	hashed_string_64 invalid_hash;

	

	static thread_local const glm::mat4 IDENTITY(1.0f);
	//auto transform_group = scene->get_registry().group<
	//	TransformDynamicComponent
	//>();
	//auto render_group = scene->get_registry().group<
	//	MeshComponent, MaterialComponent
	//>(entt::get<TransformDynamicComponent>);
	auto render_group = scene->get_registry().group<
		RenderComponent
	>(entt::get<TransformDynamicComponent>);

	
	while (true)
	{

		timer.stop();
		game_thread_time += timer.get_time_ms();

		if (runs > 500 && false)
		{
			PRINTLN("time for moving: {}", movement / (double)runs);
			PRINTLN("time for complex worldpos: {}", complex_movement / (double)runs);
			PRINTLN("time for camera updates: {}", cameras_db / (double)runs);
			PRINTLN("time for checking load jobs: {}", mesh_tex_jobs / (double)runs);
			PRINTLN("time to gather renderrequests: {}", pre_render_requests_time / (double)runs);
			PRINTLN("time for computing and moving render requests: {}", renderrequests_time / (double)runs);
			PRINTLN("time for gathering lights: {}", lighting_time / (double)runs);
			PRINTLN("time for game thread: {}", game_thread_time / (double)runs);
			movement = 0;
			complex_movement = 0;
			cameras_db = 0;
			mesh_tex_jobs = 0;
			pre_render_requests_time = 0;
			renderrequests_time = 0;
			game_thread_time = 0;
			lighting_time = 0;
			runs = 0;
		}
		else runs++;
		//PRINTLN("game thread going to sleep: {}", runs++);
		{
			std::unique_lock<std::mutex> lock(engine_context->mutex);
			engine_context->game_thread = false;
			engine_context->cv_frame_coordinator.notify_all();
			engine_context->cv_threads.wait(lock, [this] { return (engine_context->game_thread || !engine_context->run()); });
		}
		timer.start();

		if (!engine_context->run())
		{
			break;
		}
		//PRINTLN("game thread woke up");

		std::vector<RenderRequest>& render_requests = engine_context->render_requests[std::size_t(engine_context->write_pos)];
		render_requests.reserve(previous_total_render_requests);

		auto& render_context = engine_context->render_contexts[std::size_t(engine_context->write_pos)];

		std::vector<CameraData>& unique_cameras = engine_context->cameras_render[std::size_t(engine_context->write_pos)];
		unique_cameras.reserve(previous_unique_cameras);

		std::vector<PreRenderRequest> pre_render_requests;
		pre_render_requests.reserve(previous_total_render_requests);
		std::vector<PreRenderReq> pre_render_reqs;
		pre_render_requests.reserve(previous_total_render_requests);

		std::vector<hashed_string_64> hashed_mesh_loads;
		hashed_mesh_loads.reserve(10);

		std::vector<hashed_string_64> hashed_texture_loads;
		hashed_texture_loads.reserve(10);

		std::vector<hashed_string_64> hashed_material_loads;
		hashed_material_loads.reserve(10);
		// Editor camera moving
		glm::vec3 camera_move;
		{
			camera_move.x = engine_context->editor_camera.get_camera_speed() * (engine_context->input_manager.is_key_held(EKey::D) - engine_context->input_manager.is_key_held(EKey::A)) * (float)(engine_context->delta_time);
			camera_move.z = engine_context->editor_camera.get_camera_speed() * (engine_context->input_manager.is_key_held(EKey::S) - engine_context->input_manager.is_key_held(EKey::W)) * (float)(engine_context->delta_time);
			camera_move.y = engine_context->editor_camera.get_camera_speed() * (engine_context->input_manager.is_key_held(EKey::LeftShift) - engine_context->input_manager.is_key_held(EKey::LeftControl)) * (float)(engine_context->delta_time);
		}
		if (std::abs(camera_move.x) > 0.0 || std::abs(camera_move.y) > 0.0 || std::abs(camera_move.z) > 0.0) engine_context->editor_camera.move(camera_move);

		double delta_time = engine_context->delta_time;
		engine_context->systems_context->set_delta_time(delta_time);
		if (engine_context->game_playing && !engine_context->game_paused)
		{
			game_runtime();
		}
		else
		{
			// do editor loop
			editor_time();
		}
		//PRINTLN("game thread running {}", runs++);
	}

}

void GameThread::game_runtime()
{
	auto transform_group = scene->get_registry().group<
		TransformDynamicComponent
	>();
	auto render_group = scene->get_registry().group<
		RenderComponent
	>(entt::get<TransformDynamicComponent>);

	std::vector<RenderRequest>& render_requests = engine_context->render_requests[std::size_t(engine_context->write_pos)];
	render_requests.reserve(previous_total_render_requests);

	auto& render_context = engine_context->render_contexts[std::size_t(engine_context->write_pos)];

	std::vector<CameraData>& unique_cameras = engine_context->cameras_render[std::size_t(engine_context->write_pos)];
	unique_cameras.reserve(previous_unique_cameras);

	std::vector<PreRenderRequest> pre_render_requests;
	pre_render_requests.reserve(previous_total_render_requests);
	std::vector<PreRenderReq> pre_render_reqs;
	pre_render_requests.reserve(previous_total_render_requests);

	std::vector<hashed_string_64> hashed_mesh_loads;
	hashed_mesh_loads.reserve(10);

	std::vector<hashed_string_64> hashed_texture_loads;
	hashed_texture_loads.reserve(10);

	std::vector<hashed_string_64> hashed_material_loads;
	hashed_material_loads.reserve(10);

	auto& registry = scene->get_registry(); // get registry from the scene

	ind_timer.start();
	// this about 1ms
	//accumilated_time += engine_context->systems_context->get_delta_time();
	//for (auto [entity, transform, spawn]
	//	: registry.view<TransformDynamicComponent, SpawnPositionComponent>().each())
	//{
	//	//worldpos.position.x += 0.1f * (float)delta_time;
	//	//worldpos.rotation_quat *= glm::quat(glm::radians(glm::vec3(0.1, 0.1, 0.1) * 50.f * (float)delta_time));
	//	float amplitude = 5.f;
	//	float frequency = 5.f;
	//	float offset = amplitude * glm::sin(frequency * transform.position.x + accumilated_time);
	//	transform.position.y = spawn.spawn_position.y + offset;
	//}
	bool physics_frame = engine_context->physics_frame;
	auto& systems = scene->get_systems();
	for (auto& system : systems)
	{
		bool only_on_physics = system->get_physics_frames_only();
		if (system->get_enabled() && (only_on_physics == (physics_frame && only_on_physics)))
		{
			//if (system->execute(*engine_context->systems_context))
			//{
			//	PRINTLN("synced");
			//}
			system->execute(*engine_context->systems_context);
		}
	}
	//PRINTLN("started parallel");
	//move_system.execute(*engine_context->systems_context);
	//PRINTLN("done parallel");
	//engine_context->systems_context->sync();
	ind_timer.stop();
	movement += ind_timer.get_time_ms();

	ind_timer.start();
	// This takes like 2ms
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Entity rotation and position final computation (also handles what the imgui has hooked)
	// need to be done only on root entities first, then do itterative to children and siblings
	//for (auto [entity, world_pos] : registry.view<TransformDynamicComponent>().each()) // this is very multi threadable
	//{
	//	//for (auto [entity, world_pos] : transform_group.each()) // this is very multi threadable
	//	//{
	//	if (world_pos.overide_quaternion)
	//	{
	//		world_pos.rotation_euler_do_not_use.x = std::fmod(world_pos.rotation_euler_do_not_use.x, 360.0f);
	//		if (world_pos.rotation_euler_do_not_use.x < 0.0f) world_pos.rotation_euler_do_not_use.x += 360.0f;
	//		world_pos.rotation_euler_do_not_use.y = std::fmod(world_pos.rotation_euler_do_not_use.y, 360.0f);
	//		if (world_pos.rotation_euler_do_not_use.y < 0.0f) world_pos.rotation_euler_do_not_use.y += 360.0f;
	//		world_pos.rotation_euler_do_not_use.z = std::fmod(world_pos.rotation_euler_do_not_use.z, 360.0f);
	//		if (world_pos.rotation_euler_do_not_use.z < 0.0f) world_pos.rotation_euler_do_not_use.z += 360.0f;

	//		world_pos.rotation_quat = glm::quat(glm::radians(world_pos.rotation_euler_do_not_use));
	//	}
	//	if (world_pos.get_euler_angles) /// 0.5ms
	//	{
	//		world_pos.rotation_euler_do_not_use = glm::degrees(glm::eulerAngles(world_pos.rotation_quat));
	//		//if (world_pos.overide_quaternion)
	//		//	world_pos.get_euler_angles = false;
	//	}

	//	world_pos.model_matrix = glm::translate(glm::mat4(1.0f), world_pos.position); // 0.47ms
	//	world_pos.model_matrix *= glm::mat4_cast(world_pos.rotation_quat); // 1.47ms
	//	world_pos.model_matrix = glm::scale(world_pos.model_matrix, world_pos.scale); // 0.4ms
	//}
	transform_calculation_system.execute(*engine_context->systems_context);
	ind_timer.stop();
	complex_movement += ind_timer.get_time_ms();

	
	// all of this takes like 2ms
	//if (runs == 1000)
	//{
	////////////////////////////
	// Rendering and requests
	ind_timer.start();
	for (auto [entity, mesh_component] : registry.view<MeshComponent>().each())
	{
		if (!mesh_component.loaded)
		{
			// fecthing all entities with a mesh
			hashed_string_64 temp(mesh_component.hashed_path);
			mesh_component.hashed_path = temp;
			hashed_mesh_loads.push_back(temp);
			mesh_component.loaded = true;
		}
	}

	///////////////////////////////////////////////////////
	// filter meshes to only those that are unique... send then to load, but they might already be loaded
	if (!hashed_mesh_loads.empty())
	{
		// all load requests
		std::sort(hashed_mesh_loads.begin(), hashed_mesh_loads.end(), std::less<>{});
		std::vector<hashed_string_64> load_requests;
		load_requests.reserve(previous_unique_meshes);

		// unique load requests
		hashed_string_64 previous_value = hashed_mesh_loads[0];
		load_requests.push_back(previous_value);
		size_t i = 1;
		for (; i < hashed_mesh_loads.size(); i++)
		{
			if (previous_value < hashed_mesh_loads[i])
			{
				load_requests.push_back(hashed_mesh_loads[i]);
				previous_value = hashed_mesh_loads[i];
			}
		}

		//std::vector<LoadJob> load_jobs;
		std::vector<AssetJob> load_jobs;
		load_jobs.reserve(previous_unique_meshes);
		for (size_t j = 0; j < load_requests.size(); j++)
		{
			MeshDataJob job(load_requests[j], ERequestType::LoadFromFile);
			load_jobs.emplace_back(std::move(job));
		}
		previous_unique_meshes = i;
		//engine_context->model_storage->add_jobs(load_jobs);
		//for (size_t i = 0; i < load_jobs.size(); i++)
		//{
		//	engine_context->asset_manager->add_asset_job(load_jobs[i]);
		//}
		engine_context->asset_manager->add_asset_jobs(load_jobs);
	}
	///////////////////////////////////////////////////////
	// Do texture lodas now
	//for (auto [entity, texture_component] : registry.view<TextureComponent>().each())
	//{
	//	if (!texture_component.loaded)
	//	{
	//		texture_component.loaded = true;
	//		hashed_texture_loads.push_back(texture_component.hashed_path);
	//	}
	//}
	//if (!hashed_texture_loads.empty())
	//{
	//	// all load requests
	//	std::sort(hashed_texture_loads.begin(), hashed_texture_loads.end(), std::less<>{});
	//	std::vector<hashed_string_64> load_requests;
	//	load_requests.reserve(previous_unique_meshes);

	//	// unique load requests
	//	hashed_string_64 previous_value = hashed_texture_loads[0];
	//	load_requests.push_back(previous_value);
	//	size_t i = 1;
	//	for (; i < hashed_texture_loads.size(); i++)
	//	{
	//		if (previous_value < hashed_texture_loads[i])
	//		{
	//			load_requests.push_back(hashed_texture_loads[i]);
	//			previous_value = hashed_texture_loads[i];
	//		}
	//	}

	//	std::vector<LoadJob> load_jobs;
	//	load_jobs.reserve(previous_unique_meshes);
	//	for (size_t j = 0; j < load_requests.size(); j++)
	//	{
	//		TextureDataJob job(load_requests[j], ERequestType::LoadFromFile);
	//		load_jobs.emplace_back(std::move(job));
	//	}
	//	engine_context->model_storage->add_jobs(load_jobs);
	//}

	for (auto [entity, material_component] : registry.view<MaterialComponent>().each())
	{
		if (material_component.load)
		{
			material_component.load = false;
			hashed_material_loads.push_back(material_component.hashed_path);
		}
	}
	if (!hashed_material_loads.empty())
	{
		// all load requests
		std::sort(hashed_material_loads.begin(), hashed_material_loads.end(), std::less<>{});
		std::vector<hashed_string_64> load_requests;
		load_requests.reserve(previous_unique_meshes);

		// unique load requests
		hashed_string_64 previous_value = hashed_material_loads[0];
		load_requests.push_back(previous_value);
		size_t i = 1;
		for (; i < hashed_material_loads.size(); i++)
		{
			if (previous_value < hashed_material_loads[i])
			{
				load_requests.push_back(hashed_material_loads[i]);
				previous_value = hashed_material_loads[i];
			}
		}

		std::vector<AssetJob> load_jobs;
		load_jobs.reserve(previous_unique_meshes);
		for (size_t j = 0; j < load_requests.size(); j++)
		{
			MaterialDataJob job(load_requests[j], ERequestType::LoadFromFile);
			load_jobs.emplace_back(std::move(job));
		}
		engine_context->asset_manager->add_asset_jobs(load_jobs);
	}
	//	runs = 0;
	//}
	//runs++;
	ind_timer.stop();
	mesh_tex_jobs += ind_timer.get_time_ms();

	engine_context->systems_context->gen_sync();
	engine_context->systems_context->entt_sync();

	ind_timer.start();
	/////////////////////////////////////////////////////
	//// start creating render requests
	/// This must change, it must take in mesh path and texture path ... later material path instead of texture path
	//for (auto [entity, mesh_comoponent,texture_component, world_position]
	//	: render_group.each())//: registry.view<TransformDynamicComponent, MeshComponent, TextureComponent>().each())
	//{
	//	pre_render_requests.emplace_back(world_position.model_matrix, mesh_comoponent.hashed_path.hash, texture_component.hashed_path.hash);
	//}
	//for (auto [entity, world_position, mesh_comoponent]
	//	: registry.view<TransformDynamicComponent, MeshComponent>(entt::exclude<TextureComponent>).each())
	//{
	//	//if (auto tex = registry.try_get<TextureComponent>(entity))
	//	//{
	//	//	pre_render_requests.emplace_back(world_position.model_matrix, mesh_comoponent.hashed_path.hash, tex->hashed_path.hash);
	//	//}
	//	//else
	//	//{
	//		pre_render_requests.emplace_back(world_position.model_matrix, mesh_comoponent.hashed_path.hash, invalid_hash.hash);
	//	//}
	//	
	//}

	//for (auto [entity, mesh_component, material_component, world_position]
	//	: render_group.each())//registry.view<MeshComponent, MaterialComponent, TransformDynamicComponent>().each())
	//{
	//	PreRenderReq pre;
	//	pre_render_reqs.emplace_back(world_position.model_matrix, mesh_component.hashed_path.hash,
	//		material_component.hashed_path.hash, material_component.instance, material_component.mipmap);
	//}

	for (auto [entity, render, transform] :
		//registry.view<RenderComponent, TransformDynamicComponent>().each())
		render_group.each())
	{
		pre_render_reqs.emplace_back(transform.model_matrix, render.mesh.hash,
			render.material.hash, render.instance, render.mipmap);
	}

	ind_timer.stop();
	pre_render_requests_time += ind_timer.get_time_ms();

	ind_timer.start();
	////////////////////////////
	// Fetch cameras
	std::vector<CameraData> cameras;
	cameras.reserve(10); /////// should base this on something instead
	glm::vec3 direction;
	glm::vec3 right;
	glm::vec3 up;
	for (auto [entity, world_pos, camera_comp] : registry.view<TransformDynamicComponent, CameraComponent>().each())
	{
		if (camera_comp.wants_to_render)
		{
			if (camera_comp.orbit_camera)
			{
				if (camera_comp.target_entity.entity != entt::null)
				{
					if (auto target_world_pos = registry.try_get<TransformDynamicComponent>(camera_comp.target_entity.entity))
						camera_comp.target_location = target_world_pos->position;
					else
					{
						PRINTLN("Failed to get target loction, whaaaat.");
					}
				}
				direction = glm::normalize(world_pos.position - camera_comp.target_location);
				right = glm::normalize(glm::cross(direction, glm::vec3(0.f, 1.f, 0.f)));
				up = glm::cross(right, direction);
				camera_comp.view_matrix = glm::lookAt(world_pos.position, camera_comp.target_location, up);
			}
			else
			{
				glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), -world_pos.position);
				glm::mat4 rotation_matrix = glm::mat4_cast(world_pos.rotation_quat);
				camera_comp.view_matrix = glm::transpose(rotation_matrix) * translation_matrix;
			}
			camera_comp.projection_matrix = glm::perspective(glm::radians(camera_comp.field_of_view), camera_comp.aspect_ratio, 0.1f, 300.0f);
			cameras.emplace_back(camera_comp.view_matrix, camera_comp.projection_matrix, camera_comp.frame_buffer_target, camera_comp.render_priority, true, true, world_pos.position);
		}
	}
	if (!cameras.empty())
	{
		std::sort(cameras.begin(), cameras.end(), [](const CameraData& cam_a, const CameraData& cam_b)
			{
				if (cam_a.frame_buffer_hashed == cam_b.frame_buffer_hashed)
					return cam_a.priority > cam_b.priority;
				return cam_a.frame_buffer_hashed < cam_b.frame_buffer_hashed;
			});
		hashed_string_64 prev_target = cameras[0].frame_buffer_hashed;
		unique_cameras.push_back(cameras[0]);
		for (size_t i = 1; i < cameras.size(); i++)
		{
			if (cameras[i].frame_buffer_hashed > prev_target)
			{
				prev_target = cameras[i].frame_buffer_hashed;
				unique_cameras.push_back(cameras[i]);
			}
		}
	}
	previous_unique_cameras = unique_cameras.size();
	ind_timer.stop();
	cameras_db += ind_timer.get_time_ms();

	ind_timer.start();
	//if (auto opt_render_requests = 
	//	engine_context->model_storage->render_request_returner.return_requests(pre_render_requests))
	//	
	//{
	//	render_requests = std::move(opt_render_requests.value());
	//}
	//render_requests = std::move(engine_context->asset_manager->retrieve_render_requests(pre_render_requests));
	render_context = std::move(engine_context->asset_manager->construct_render_context(pre_render_reqs));
	ind_timer.stop();
	renderrequests_time += ind_timer.get_time_ms();

	ind_timer.start();


	for (auto [entity, directional_light, transform] : registry.view<DirectionalLightComponent, TransformDynamicComponent>().each())
	{
		if (directional_light.shadow_map)
		{
			ShadowLight light;
			light.color = glm::vec4(directional_light.color, directional_light.intensity);
			light.lightspace_matrix = glm::mat4(1);
			light.rotation = -transform.rotation_euler_do_not_use;
			light.lightspace_matrix *= -glm::mat4_cast(transform.rotation_quat);
			render_context.shadow_lights.push_back(light);
		}
	}

	for (auto [entity, point_light, transform] : registry.view<PointLightComponent, TransformDynamicComponent>().each())
	{
		if (!point_light.shadow_map)
		{
			PointLightSSBO light;
			light.light_color = glm::vec4(point_light.color, point_light.intensity);
			light.light_attenuation = glm::vec4(point_light.attenuation, 0);
			light.light_positoin = glm::vec4(transform.position, 0);
			render_context.lights.push_back(light);
		}
	}
	ind_timer.stop();
	lighting_time += ind_timer.get_time_ms();

	for (auto [entity, mat_float] :
		registry.view<MaterialFloatComponent>().each())
	{
		if (mat_float.set)
		{
			mat_float.set = false;
			MaterialUniformJob new_uni_job{ mat_float.material, mat_float.location, mat_float.value, typeid(float) };
			AssetJob new_job = std::move(new_uni_job);
			engine_context->asset_manager->add_asset_job(new_job);
		}
		else if (mat_float.continous_update && (mat_float.prev_value != mat_float.value))
		{
			MaterialUniformJob new_uni_job{ mat_float.material, mat_float.location, mat_float.value, typeid(float) };
			AssetJob new_job = std::move(new_uni_job);
			engine_context->asset_manager->add_asset_job(new_job);
		}
	}

	for (auto [entity, mat_float] :
		registry.view<MaterialIntComponent>().each())
	{
		if (mat_float.set)
		{
			mat_float.set = false;
			int val = (int)mat_float.value;
			MaterialUniformJob new_uni_job{ mat_float.material, mat_float.location, val, typeid(int) };
			AssetJob new_job = std::move(new_uni_job);
			engine_context->asset_manager->add_asset_job(new_job);
		}
	}

	previous_total_render_requests = render_requests.size();
	/////////////////////////////////////// BELOW is for testing when comopnents are suddenly destroyed.
	//std::vector<entt::entity> entities;
	//for (auto [entity, worldpos] : registry.view<TransformDynamicComponent>().each())
	//{
	//	if (worldpos.position.x > 5)
	//	{
	//		entities.push_back(entity);
	//	}
	//}
	//for (size_t i = 0; i < entities.size(); i++)
	//{
	//	registry.remove<TransformDynamicComponent>(entities[i]);
	//}

	//for (auto [entity, testcomp] : registry.view<TestComponent>().each())
	//{
	//	testcomp.test += 1;
	//	if (testcomp.test == prev_test)
	//	{
	//		PRINTLN("entt entity component changed");
	//	}
	//	//PRINTLN("testcomp val: {}", testcomp.test);
	//	prev_test = testcomp.test;  
	//}
	//uint32_t countererer = 0;
	//for (auto [entity, name_comp] : registry.view<EntityComponent>().each())
	//{
	//	countererer++;
	//}
	//PRINTLN("This many entities: {}", countererer);

	//engine_context->systems_context->gen_sync();
	//engine_context->systems_context->entt_sync();
}

void GameThread::editor_time()
{
	std::vector<RenderRequest>& render_requests = engine_context->render_requests[std::size_t(engine_context->write_pos)];
	render_requests.reserve(previous_total_render_requests);

	auto& render_context = engine_context->render_contexts[std::size_t(engine_context->write_pos)];

	std::vector<CameraData>& unique_cameras = engine_context->cameras_render[std::size_t(engine_context->write_pos)];
	unique_cameras.reserve(previous_unique_cameras);

	std::vector<PreRenderRequest> pre_render_requests;
	pre_render_requests.reserve(previous_total_render_requests);
	std::vector<PreRenderReq> pre_render_reqs;
	pre_render_requests.reserve(previous_total_render_requests);

	std::vector<hashed_string_64> hashed_mesh_loads;
	hashed_mesh_loads.reserve(10);

	std::vector<hashed_string_64> hashed_texture_loads;
	hashed_texture_loads.reserve(10);

	std::vector<hashed_string_64> hashed_material_loads;
	hashed_material_loads.reserve(10);

	
	
	
	// maybe also all materials ??? what? why all materials?

	auto& scene_objects = scene->get_scene_objects();
	std::vector<TransformDynamicComponent*> transforms;
	std::vector<RenderComponent*> render_components;
	
	auto temp_num_scene_objects = scene_objects.size();

	std::vector<EditorEntity> editor_entities;
	editor_entities.reserve(temp_num_scene_objects);
	std::vector<hashed_string_64> materials;
	materials.reserve(temp_num_scene_objects);
	std::vector<hashed_string_64> meshes;
	meshes.reserve(temp_num_scene_objects);
	uint64_t invalid_hash = hashed_string_64("").hash;

	// multi threading is apparently slower.
	//{
	//	auto& ctx = engine_context->systems_context;
	//	size_t num_threads = engine_context->systems_context->num_threads();
	//	size_t num_scene_objects = scene_objects.size();
	//	size_t start_index = 0;
	//	size_t end_index = 0;

	//	size_t chunk_size = 1024;

	//	size_t num_chunks = (num_scene_objects / chunk_size) + 1;
	//	std::vector<std::vector<EditorEntity>> entity_containers;
	//	entity_containers.resize(num_chunks);
	//	std::vector<std::vector<hashed_string_64>> materials_containers;
	//	materials_containers.resize(num_chunks);
	//	std::vector<std::vector<hashed_string_64>> meshes_containers;
	//	meshes_containers.resize(num_chunks);

	//	for (size_t i = 0; i < num_chunks; i++)
	//	{
	//		entity_containers[i].reserve(temp_num_scene_objects);
	//		materials_containers[i].reserve(temp_num_scene_objects);
	//		meshes_containers[i].reserve(temp_num_scene_objects);
	//	}


	//	for (size_t i = 0; i < num_chunks; i++)
	//	{
	//		size_t thread_id = i;

	//		ctx->enqueue([&, thread_id, chunk_size, num_scene_objects]()
	//			{
	//				auto& materials_container = materials_containers[thread_id];
	//				auto& meshes_container = meshes_containers[thread_id];
	//				auto& entity_container = entity_containers[thread_id];
	//				uint64_t invalid_hash = hashed_string_64("").hash;
	//				for (size_t i = thread_id * chunk_size; i <((thread_id + 1) * chunk_size) && i < num_scene_objects; i++)
	//				{
	//					auto& scene_object = scene_objects[i];
	//					bool valid_entity = false;
	//					EditorEntity entity;

	//					auto& component_datas = scene_object->get_component_datas();
	//					for (auto& component_data : component_datas)
	//					{

	//						std::type_index type = component_data->get_type();

	//						if (type == typeid(TransformDynamicComponent))
	//						{
	//							ComponentData<TransformDynamicComponent>* data = static_cast<ComponentData<TransformDynamicComponent>*>(component_data.get());
	//							entity.transform = &data->get_component();
	//							valid_entity = true;
	//						}
	//						else if (type == typeid(RenderComponent))
	//						{
	//							ComponentData<RenderComponent>* data = static_cast<ComponentData<RenderComponent>*>(component_data.get());
	//							entity.render = &data->get_component();
	//							valid_entity = true;
	//							if (entity.render->material.hash != invalid_hash)
	//							{
	//								materials_container.push_back(entity.render->material);
	//							}
	//							if (entity.render->mesh.hash != invalid_hash)
	//							{
	//								meshes_container.push_back(entity.render->mesh);
	//							}
	//						}
	//						else if (type == typeid(PointLightComponent))
	//						{
	//							ComponentData<PointLightComponent>* data = static_cast<ComponentData<PointLightComponent>*>(component_data.get());
	//							entity.point_light = &data->get_component();
	//							valid_entity = true;
	//						}
	//					}
	//					if (valid_entity)
	//					{
	//						entity_container.push_back(entity);
	//					}
	//				}
	//			});
	//	}
	//	ctx->sync();
	//	for (size_t i = 0; i < materials_containers.size(); i++)
	//	{
	//		for (size_t j = 0; j < materials_containers[i].size(); j++)
	//		{
	//			materials.emplace_back(materials_containers[i][j]);
	//		}
	//	}
	//	for (size_t i = 0; i < meshes_containers.size(); i++)
	//	{
	//		for (size_t j = 0; j < meshes_containers[i].size(); j++)
	//		{
	//			meshes.emplace_back(meshes_containers[i][j]);
	//		}
	//	}
	//	for (size_t i = 0; i < entity_containers.size(); i++)
	//	{
	//		for (size_t j = 0; j < entity_containers[i].size(); j++)
	//		{
	//			editor_entities.emplace_back(entity_containers[i][j]);
	//		}
	//	}
	//}

	for (auto& scene_object : scene_objects)
	{
		bool valid_entity = false;
		EditorEntity entity;

		auto& component_datas = scene_object->get_component_datas();
		for (auto& component_data : component_datas)
		{
			
			std::type_index type = component_data->get_type();
			
			if (type == typeid(TransformDynamicComponent))
			{
				ComponentData<TransformDynamicComponent>* data = static_cast<ComponentData<TransformDynamicComponent>*>(component_data.get());
				entity.transform = &data->get_component();
				valid_entity = true;
			}
			else if (type == typeid(RenderComponent))
			{
				ComponentData<RenderComponent>* data = static_cast<ComponentData<RenderComponent>*>(component_data.get());
				entity.render = &data->get_component();
				valid_entity = true;
				if (entity.render->material.hash != invalid_hash)
				{
					materials.push_back(entity.render->material);
				}
				if (entity.render->mesh.hash != invalid_hash)
				{
					meshes.push_back(entity.render->mesh);
				}
			}
			else if (type == typeid(PointLightComponent))
			{
				ComponentData<PointLightComponent>* data = static_cast<ComponentData<PointLightComponent>*>(component_data.get());
				entity.point_light = &data->get_component();
				valid_entity = true;
			}
		}
		if (valid_entity)
		{
			editor_entities.push_back(entity);
		}
	}

	// then compare the mats/meshes with what's been confirmed loaded, load all that haven't been
	auto& requested_mats = engine_context->requested_assets.materials;
	auto& requested_meshes = engine_context->requested_assets.meshes;
	std::sort(materials.begin(), materials.end(), [](const hashed_string_64& hash_a, const hashed_string_64& hash_b)
		{
			return hash_a.hash < hash_b.hash;
		});
	auto dupes = std::unique(materials.begin(), materials.end(), [](const hashed_string_64& hash_a, const hashed_string_64& hash_b)
		{
			return hash_a.hash == hash_b.hash;
		});
	materials.erase(dupes, materials.end());

	std::sort(meshes.begin(), meshes.end(), [](const hashed_string_64& hash_a, const hashed_string_64& hash_b)
		{
			return hash_a.hash < hash_b.hash;
		});
	dupes = std::unique(meshes.begin(), meshes.end(), [](const hashed_string_64& hash_a, const hashed_string_64& hash_b)
		{
			return hash_a.hash == hash_b.hash;
		});
	meshes.erase(dupes, meshes.end());


	// start making laod jobs
	if (!meshes.empty())
	{
		std::vector<hashed_string_64> load_requests;
		load_requests.reserve(previous_unique_meshes);

		size_t mesh_index = 0;
		size_t r_mesh_index = 0;
		while (mesh_index < meshes.size())
		{
			auto& mesh = meshes[mesh_index];

			// Advance r_mesh_index until we find a match or pass the current mesh
			while (r_mesh_index < requested_meshes.size() &&
				requested_meshes[r_mesh_index].hash < mesh.hash)
			{
				r_mesh_index++;
			}

			// Check if we found a match
			bool found_match = (r_mesh_index < requested_meshes.size() &&
				requested_meshes[r_mesh_index].hash == mesh.hash);

			if (!found_match)
			{
				load_requests.push_back(mesh);
			}
			mesh_index++;
		}

		std::vector<AssetJob> load_jobs;
		load_jobs.reserve(10);
		for (size_t j = 0; j < load_requests.size(); j++)
		{
			MeshDataJob job(load_requests[j], ERequestType::LoadFromFile);
			requested_meshes.push_back(load_requests[j]);
			load_jobs.emplace_back(std::move(job));
		}
		engine_context->asset_manager->add_asset_jobs(load_jobs);

		if (!load_requests.empty())
		{
			std::sort(requested_meshes.begin(), requested_meshes.end(), [](const hashed_string_64& hash_a, const hashed_string_64& hash_b)
				{
					return hash_a.hash < hash_b.hash;
				});
			PRINTLN("Added meshes");
		}
	}

	if (!materials.empty())
	{
		std::vector<hashed_string_64> load_requests;
		load_requests.reserve(previous_unique_meshes);

		size_t mat_index = 0;
		size_t r_mat_index = 0;
		while (mat_index < materials.size())
		{
			auto& mat = materials[mat_index];

			// Advance r_mesh_index until we find a match or pass the current mesh
			while (r_mat_index < requested_mats.size() &&
				requested_mats[r_mat_index].hash < mat.hash)
			{
				r_mat_index++;
			}

			// Check if we found a match
			bool found_match = (r_mat_index < requested_mats.size() &&
				requested_mats[r_mat_index].hash == mat.hash);

			if (!found_match)
			{
				load_requests.push_back(mat);
			}
			mat_index++;
		}

		std::vector<AssetJob> load_jobs;
		load_jobs.reserve(10);
		for (size_t j = 0; j < load_requests.size(); j++)
		{
			MaterialDataJob job(load_requests[j], ERequestType::LoadFromFile);
			requested_mats.push_back(load_requests[j]);
			load_jobs.emplace_back(std::move(job));
		}
		engine_context->asset_manager->add_asset_jobs(load_jobs);

		if (!load_requests.empty())
		{
			std::sort(requested_mats.begin(), requested_mats.end(), [](const hashed_string_64& hash_a, const hashed_string_64& hash_b)
				{
					return hash_a.hash < hash_b.hash;
				});
			PRINTLN("Added mats");
		}
	}

	// update all the stuff like in ecs for transform
	for (auto& entity : editor_entities)
	{
		auto& world_pos = *entity.transform;
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
	}

	// generate all the render requests
	for ( auto& entity: editor_entities)
	{
		if (entity.transform && entity.render)
		{
			auto& transform = *entity.transform;
			auto& render = *entity.render;
			pre_render_reqs.emplace_back(transform.model_matrix, render.mesh.hash,
				render.material.hash, render.instance, render.mipmap);
		}
	}
	render_context = std::move(engine_context->asset_manager->construct_render_context(pre_render_reqs));

	// also get all the lights
	for (auto& entity : editor_entities)
	{
		if (entity.point_light && entity.transform)
		{
			auto& point_light = *entity.point_light;
			auto& transform = *entity.transform;
			if (point_light.shadow_map) continue;

			PointLightSSBO light;
			light.light_color = glm::vec4(point_light.color, point_light.intensity);
			light.light_attenuation = glm::vec4(point_light.attenuation, 0);
			light.light_positoin = glm::vec4(transform.position, 0);
			render_context.lights.push_back(light);
		}
	}

}
