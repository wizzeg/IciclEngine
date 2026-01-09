#include "worker_thread.h"
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

void GameThread::execute()
{
	uint64_t prev_test = 0;
	int runs = 0;
	size_t previous_total_render_requests = 10;
	size_t previous_unique_meshes = 10;
	size_t previous_unique_cameras = 2;
	hashed_string_64 invalid_hash;

	HighResolutionTimer timer;
	HighResolutionTimer ind_timer;
	double movement = 0;
	double complex_movement = 0;
	double cameras_db = 0;
	double mesh_tex_jobs = 0;
	double pre_render_requests_time = 0;
	double renderrequests_time = 0;
	double game_thread_time = 0;
	static thread_local const glm::mat4 IDENTITY(1.0f);
	auto transform_group = scene->get_registry().group<
		TransformDynamicComponent
	>();
	auto render_group = scene->get_registry().group<
		MeshComponent, TextureComponent
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
			PRINTLN("time for game thread: {}", game_thread_time / (double)runs);
			movement = 0;
			complex_movement = 0;
			cameras_db = 0;
			mesh_tex_jobs = 0;
			pre_render_requests_time = 0;
			renderrequests_time = 0;
			game_thread_time = 0;
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
		if (engine_context->game_playing)
		{
			auto& registry = scene->get_registry(); // get registry from the scene

			ind_timer.start();
			// this about 1ms
			//for (auto [entity, name, worldpos] 
			//	: registry.view<NameComponent, TransformDynamicComponent>(entt::exclude<CameraComponent>).each())
			//{
			//	worldpos.position.x += 0.1f * (float)delta_time;
			//	worldpos.rotation_quat *= glm::quat(glm::radians(glm::vec3(0.1, 0.1, 0.1) * 50.f * (float)delta_time));
			//}
			ind_timer.stop();
			movement += ind_timer.get_time_ms();

			ind_timer.start();
			// This takes like 2ms
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// Entity rotation and position final computation (also handles what the imgui has hooked)
			// need to be done only on root entities first, then do itterative to children and siblings
			for (auto [entity, world_pos] : registry.view<TransformDynamicComponent>().each()) // this is very multi threadable
			{
			//for (auto [entity, world_pos] : transform_group.each()) // this is very multi threadable
			//{
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
			ind_timer.stop();
			complex_movement += ind_timer.get_time_ms();
			

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
					cameras.emplace_back(camera_comp.view_matrix, camera_comp.projection_matrix, camera_comp.frame_buffer_target, camera_comp.render_priority, true, true, world_pos.position );
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
				for (auto [entity, texture_component] : registry.view<TextureComponent>().each())
				{
					if (!texture_component.loaded)
					{
						texture_component.loaded = true;
						hashed_texture_loads.push_back(texture_component.hashed_path);
					}
				}
				if (!hashed_texture_loads.empty())
				{
					// all load requests
					std::sort(hashed_texture_loads.begin(), hashed_texture_loads.end(), std::less<>{});
					std::vector<hashed_string_64> load_requests;
					load_requests.reserve(previous_unique_meshes);

					// unique load requests
					hashed_string_64 previous_value = hashed_texture_loads[0];
					load_requests.push_back(previous_value);
					size_t i = 1;
					for (; i < hashed_texture_loads.size(); i++)
					{
						if (previous_value < hashed_texture_loads[i])
						{
							load_requests.push_back(hashed_texture_loads[i]);
							previous_value = hashed_texture_loads[i];
						}
					}

					std::vector<LoadJob> load_jobs;
					load_jobs.reserve(previous_unique_meshes);
					for (size_t j = 0; j < load_requests.size(); j++)
					{
						TextureDataJob job(load_requests[j], ERequestType::LoadFromFile);
						load_jobs.emplace_back(std::move(job));
					}
					engine_context->model_storage->add_jobs(load_jobs);
				}

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
				

				ind_timer.start();
			/////////////////////////////////////////////////////
			//// start creating render requests
			/// This must change, it must take in mesh path and texture path ... later material path instead of texture path
			for (auto [entity, mesh_comoponent,texture_component, world_position]
				: render_group.each())//: registry.view<TransformDynamicComponent, MeshComponent, TextureComponent>().each())
			{
				pre_render_requests.emplace_back(world_position.model_matrix, mesh_comoponent.hashed_path.hash, texture_component.hashed_path.hash);
			}
			for (auto [entity, world_position, mesh_comoponent]
				: registry.view<TransformDynamicComponent, MeshComponent>(entt::exclude<TextureComponent>).each())
			{
				//if (auto tex = registry.try_get<TextureComponent>(entity))
				//{
				//	pre_render_requests.emplace_back(world_position.model_matrix, mesh_comoponent.hashed_path.hash, tex->hashed_path.hash);
				//}
				//else
				//{
					pre_render_requests.emplace_back(world_position.model_matrix, mesh_comoponent.hashed_path.hash, invalid_hash.hash);
				//}
				
			}

			for (auto [entity, world_position, mesh_component, material_component] 
				: registry.view<TransformDynamicComponent, MeshComponent, MaterialComponent>().each())
			{
				PreRenderReq pre;
				pre_render_reqs.emplace_back(world_position.model_matrix, mesh_component.hashed_path.hash,
					material_component.hashed_path.hash, material_component.instance, material_component.mipmap);
			}

			ind_timer.stop();
			pre_render_requests_time += ind_timer.get_time_ms();

			ind_timer.start();
			//if (auto opt_render_requests = 
			//	engine_context->model_storage->render_request_returner.return_requests(pre_render_requests))
			//	
			//{
			//	render_requests = std::move(opt_render_requests.value());
			//}
			render_requests = std::move(engine_context->asset_manager->retrieve_render_requests(pre_render_requests));
			render_context = std::move(engine_context->asset_manager->construct_render_context(pre_render_reqs));
			ind_timer.stop();
			renderrequests_time += ind_timer.get_time_ms();
			
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
				//for (auto [entity, name_comp] : registry.view<NameComponent>().each())
				//{
				//	countererer++;
				//}
				//PRINTLN("This many entities: {}", countererer);
		}
		//PRINTLN("game thread running {}", runs++);
	}

}