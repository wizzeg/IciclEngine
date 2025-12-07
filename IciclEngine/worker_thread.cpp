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

void GameThread::execute()
{
	int runs = 0;
	size_t previous_total_render_requests = 10;
	size_t previous_unique_meshes = 10;
	size_t previous_unique_cameras = 2;
	while (true)
	{
		//PRINTLN("game thread going to sleep: {}", runs++);
		{
			std::unique_lock<std::mutex> lock(engine_context->mutex);
			engine_context->game_thread = false;
			engine_context->cv_frame_coordinator.notify_all();
			engine_context->cv_threads.wait(lock, [this] { return (engine_context->game_thread || !engine_context->run()); });
		}
		if (!engine_context->run())
		{
			break;
		}
		//PRINTLN("game thread woke up");

		std::vector<RenderRequest>& render_requests = engine_context->render_requests[std::size_t(engine_context->write_pos)];
		render_requests.reserve(previous_total_render_requests);
		std::vector<CameraData>& unique_cameras = engine_context->cameras_render[std::size_t(engine_context->write_pos)];
		unique_cameras.reserve(previous_unique_cameras);

		std::vector<PreRenderRequest> pre_render_requests;
		pre_render_requests.reserve(previous_total_render_requests);

		std::vector<hashed_string_64> hashed_loads;
		hashed_loads.reserve(previous_total_render_requests);

		// Editor camera moving
		glm::vec3 camera_move;
		{
			camera_move.x = engine_context->editor_camera.get_camera_speed() * (engine_context->input_manager.is_key_held(EKey::D) - engine_context->input_manager.is_key_held(EKey::A)) * (float)(engine_context->delta_time * 0.001);
			camera_move.z = engine_context->editor_camera.get_camera_speed() * (engine_context->input_manager.is_key_held(EKey::S) - engine_context->input_manager.is_key_held(EKey::W)) * (float)(engine_context->delta_time * 0.001);
			camera_move.y = engine_context->editor_camera.get_camera_speed() * (engine_context->input_manager.is_key_held(EKey::LeftShift) - engine_context->input_manager.is_key_held(EKey::LeftControl)) * (float)(engine_context->delta_time * 0.001);
		}
		if (std::abs(camera_move.x) > 0.0 || std::abs(camera_move.y) > 0.0 || std::abs(camera_move.z) > 0.0) engine_context->editor_camera.move(camera_move);


		if (engine_context->game_playing)
		{
			auto& registry = scene->get_registry(); // get registry from the scene

			for (auto [entity, name, worldpos] 
				: registry.view<NameComponent, WorldPositionComponent>(entt::exclude<CameraComponent>).each())
			{
				worldpos.position.x += 0.0001f;
				worldpos.rotation_quat *= glm::quat(glm::radians(glm::vec3(0.1, 0.1, 0.1)));
			}

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// Entity rotation and position final computation (also handles what the imgui has hooked)
			// need to be done only on root entities first, then do itterative to children and siblings
			for (auto [entity, world_pos] : registry.view<WorldPositionComponent>().each())
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
				if (world_pos.get_euler_angles)
				{
					world_pos.rotation_euler_do_not_use = glm::degrees(glm::eulerAngles(world_pos.rotation_quat));
					if (world_pos.overide_quaternion)
						world_pos.get_euler_angles = false;
				}

				world_pos.model_matrix = glm::translate(glm::mat4(1.0f), world_pos.position);
				world_pos.model_matrix *= glm::mat4_cast(world_pos.rotation_quat);
				world_pos.model_matrix = glm::scale(world_pos.model_matrix, world_pos.scale);
			}

			////////////////////////////
			// Fetch cameras
			std::vector<CameraData> cameras;
			cameras.reserve(10); /////// should base this on something instead
			glm::vec3 direction;
			glm::vec3 right;
			glm::vec3 up;
			for (auto [entity, world_pos, camera_comp] : registry.view<WorldPositionComponent, CameraComponent>().each())
			{
				if (camera_comp.wants_to_render)
				{
					if (camera_comp.orbit_camera)
					{
						if (camera_comp.target_entity != entt::null)
						{
							if (auto target_world_pos = registry.try_get<WorldPositionComponent>(camera_comp.target_entity))
								camera_comp.target_location = target_world_pos->position;
						}
						direction = glm::normalize(world_pos.position - camera_comp.target_location);
						right = glm::normalize(glm::cross(direction, glm::vec3(0.f, 1.f, 0.f)));
						up = glm::cross(right, direction);
						camera_comp.view_matrix = glm::lookAt(world_pos.position, camera_comp.target_location, up);
					}
					else
					{
						glm::mat4 rotation_matrix = glm::mat4_cast(world_pos.rotation_quat);
						glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), -world_pos.position);
						camera_comp.view_matrix = glm::transpose(rotation_matrix) * translation_matrix;
					}
					camera_comp.projection_matrix = glm::perspective(glm::radians(camera_comp.fied_of_view), camera_comp.aspect_ratio, 0.1f, 300.0f);
					cameras.emplace_back(true, true, camera_comp.render_priority, camera_comp.view_matrix, camera_comp.projection_matrix, camera_comp.frame_buffer_target);
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


			////////////////////////////
			// Rendering and requests

			for (auto [entity, mesh_component] : registry.view<MeshComponent>().each())
			{
				if (mesh_component.loaded == false)
				{
					// fecthing all entities with a mesh
					hashed_string_64 temp(mesh_component.hashed_path);
					mesh_component.hashed_path = temp;
					hashed_loads.push_back(temp);
					mesh_component.loaded = true;
				}
			}

			///////////////////////////////////////////////////////
			// filter meshes to only those that are unique... send then to load, but they might already be loaded
			if (!hashed_loads.empty())
			{
				// all load requests
				std::sort(hashed_loads.begin(), hashed_loads.end(), std::less<>{});
				std::vector<hashed_string_64> load_requests;
				load_requests.reserve(previous_unique_meshes);

				// unique load requests
				hashed_string_64 previous_value = hashed_loads[0];
				load_requests.push_back(previous_value);
				size_t i = 1;
				for (; i < hashed_loads.size(); i++)
				{
					if (previous_value < hashed_loads[i])
					{
						load_requests.push_back(hashed_loads[i]);
						previous_value = hashed_loads[i];
					}
				}
				// NEW MODEL LOADING SYSTEM
				std::vector<LoadJob> load_jobs;
				load_jobs.reserve(previous_unique_meshes);
				for (size_t j = 0; j < load_requests.size(); j++)
				{
					MeshDataJob job(load_requests[j], EMeshDataRequest::LoadMeshFromFile);
					load_jobs.emplace_back(std::move(job));
				}
				previous_unique_meshes = i;
				engine_context->model_storage->add_jobs(load_jobs);
			}
			///////////////////////////////////////////////////////


			/////////////////////////////////////////////////////
			//// start creating render requests
			for (auto [entity, world_position, mesh_comoponent]
				: registry.view<WorldPositionComponent, MeshComponent>().each())
			{
				pre_render_requests.emplace_back(mesh_comoponent.hashed_path, world_position.model_matrix);
			}
			// Only use unique meshes to obtain render info
			std::sort(pre_render_requests.begin(), pre_render_requests.end(),
				[](const PreRenderRequest req_a, const PreRenderRequest req_b)
				{
					return req_a.hashed_path < req_b.hashed_path;
				});
			if (!pre_render_requests.empty())
			{
				std::vector<hashed_string_64> unique_render_paths;
				hashed_string_64 previous_value = pre_render_requests[0].hashed_path;
				unique_render_paths.push_back(previous_value);
				size_t i = 1;
				for (; i < pre_render_requests.size(); i++)
				{
					if (previous_value < pre_render_requests[i].hashed_path)
					{
						unique_render_paths.push_back(pre_render_requests[i].hashed_path);
						previous_value = pre_render_requests[i].hashed_path;
					}
				}
				previous_unique_meshes = i;

				/////////////////////////////////////////////////////////
				// NEW MODEL LOADING SYSTEM
				// for each mesh object and unique render request create a render request by combinging model matrix and vao etc
				if (auto unique_render_requests_opt = engine_context->model_storage->render_request_returner.return_requests(unique_render_paths))
				{
					auto& unique_render_requests = unique_render_requests_opt.value();
					previous_unique_meshes = unique_render_paths.size();
					size_t unique_index = 0;
					size_t start_index = 0;
					bool none_found = true;
					for (auto& render_request : pre_render_requests)
					{
						none_found = true;
						start_index = unique_index;
						for (; unique_index < unique_render_requests.size(); unique_index++)
						{
							if (render_request.hashed_path == unique_render_requests[unique_index].hashed_path)
							{
								render_requests.emplace_back(
									render_request.hashed_path, unique_render_requests[unique_index].vao,
									unique_render_requests[unique_index].indices_size, render_request.model_matrix, 0, 0);
								none_found = false;
								break;
							}

						}
						if (none_found)
						{
							unique_index = start_index;
						}
					}
				}
				previous_total_render_requests = render_requests.size();
			}
		}
		//PRINTLN("game thread running {}", runs++);
	}

}