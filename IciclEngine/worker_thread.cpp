#include "worker_thread.h"
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <engine/editor/scene_object.h>
#include <engine/renderer/render_info.h>
#include <glad/glad.h>
#include <KHR/khrplatform.h>
#include <glfw/glfw3.h>

void RenderThread::execute()
{
	//if (auto window = gl_context.lock())
	//{
	//	window->activate();
	//}

	int runs = 0;
	while (true)
	{
		runs++;
		///* Swap front and back buffers */
		if (auto window = gl_context.lock())
		{
			window->swap_buffers();
			window->deactivate();
		}
		
		//PRINTLN("render thread going to sleep: {}", runs++);
		{
			std::unique_lock<std::mutex> lock(engine_context->mutex);
			engine_context->render_thread = false;
			engine_context->cv_frame_coordinator.notify_all();
			engine_context->cv_threads.wait(lock, [this] { return (engine_context->render_thread || !engine_context->run()); });
		}
		if (!engine_context->run()) break; engine_context->cv_frame_coordinator.notify_all(); engine_context->cv_threads.notify_all();
		if (auto window = gl_context.lock())
		{
			if (window->window_should_close()) engine_context->kill_all = true;
			window->activate();
			window->clear();

			auto& render_requests = engine_context->render_requests[std::size_t(!engine_context->write_pos)];
			for (size_t i = 0; i < render_requests.size(); i++)
			{
				PRINTLN("vao: {}, size: {}, entity: {}", render_requests[i].vao, render_requests[i].indices_size, render_requests[i].shader_program);
			}

			if (auto vao_request = engine_context->storage->get_vao_request())
			{
				PRINTLN("Render thread got vao request");
			}
		}
		

	}
}

void EngineThread::execute()
{
	int num_frames = 500;
	while (num_frames > 0 && engine_context->run())
	{
		{
			std::unique_lock<std::mutex> lock(engine_context->mutex);
			engine_context->cv_frame_coordinator.wait(lock, [this] 
			{ 
				return (engine_context->render_thread == false
					&& engine_context->game_thread == false) || !engine_context->run(); 
			});
		}

		{
			std::lock_guard<std::mutex> guard(engine_context->mutex);
			engine_context->swap_render_requests();
			engine_context->render_thread = true;
			engine_context->game_thread = true;

			// do all ui drawing.
			if (auto imgui = imgui_manager.lock())
			{
				imgui->new_frame();
				if (auto ui = ui_manager.lock())
				{
					ui->draw_object_hierarchy();
					ui->draw_object_properties();
				}
				imgui->render();
				
			}
		}
		engine_context->cv_threads.notify_all();
	}
}

void GameThread::execute()
{
	int runs = 0;
	size_t previous_total_render_requests = 10;
	size_t previous_unique_meshes = 10;
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

				//PRINTLN("game thread going to sleep: {}", runs++);
		std::vector<RenderRequest>& render_requests = engine_context->render_requests[std::size_t(engine_context->write_pos)];

		std::vector<PreRenderRequest> pre_render_requests;
		pre_render_requests.reserve(previous_total_render_requests);

		std::vector<entt::hashed_string> hashed_loads;
		hashed_loads.reserve(previous_total_render_requests);
		if (engine_context->game_playing)
		{
			auto& registry = scene->get_registry();

			for (auto [entity, name, worldpos] : registry.view<NameComponent, WorldPositionComponent>().each())
			{
				worldpos.position.x += 0.0001f;
			}


			////////////////////////////
			// Rendering and requests

			for (auto [entity, mesh_component] : registry.view<MeshComponent>().each())
			{
				if (mesh_component.id == 0)
				{
					// fecthing all entities with a mesh
					hashed_loads.push_back(mesh_component.hashed_path);
					mesh_component.id = 1;
				}
			}

			///////////////////////////////////////////////////////
			// filter meshes to only those that are unique... send then to load, but they might already be loaded
			if (!hashed_loads.empty())
			{
				// all load requests
				std::sort(hashed_loads.begin(), hashed_loads.end(), std::less<>{});
				std::vector<entt::hashed_string> load_requests;
				load_requests.reserve(previous_unique_meshes);

				// unique load requests
				entt::hashed_string previous_value = hashed_loads[0];
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
				previous_unique_meshes = i;
				engine_context->storage->load_requests(load_requests);
			}
			///////////////////////////////////////////////////////


			/////////////////////////////////////////////////////
			//// start creating render requests
			for (auto [entity, world_position, mesh_comoponent]
				: registry.view<WorldPositionComponent, MeshComponent>().each())
			{
				glm::mat4 model = glm::mat4(1);
				model = glm::translate(model, world_position.position);
				pre_render_requests.emplace_back(mesh_comoponent.hashed_path, model);
			}
			// Only use unique meshes to obtain render info
			std::sort(pre_render_requests.begin(), pre_render_requests.end(),
				[](const PreRenderRequest req_a, const PreRenderRequest req_b)
				{
					return req_a.hashed_path < req_b.hashed_path;
				});
			if (!pre_render_requests.empty())
			{
				std::vector<entt::hashed_string> unique_render_paths;
				entt::hashed_string previous_value = pre_render_requests[0].hashed_path;
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
				std::vector<RenderRequest> unique_render_requests = engine_context->storage->get_render_request(unique_render_paths);
				previous_unique_meshes = unique_render_paths.size();
				size_t unique_index = 0;
				for (auto& render_request : pre_render_requests)
				{
					for (; unique_index < unique_render_requests.size(); unique_index++)
					{
						if (render_request.hashed_path == unique_render_requests[unique_index].hashed_path)
						{
							render_requests.push_back(
								RenderRequest{ render_request.hashed_path, unique_render_requests[unique_index].vao,
								unique_render_requests[unique_index].indices_size, render_request.model_matrix, 0, 0 });
							break;
						}

					}
				}
				previous_total_render_requests = render_requests.size();
			}
		}
		//else
		//{
		//	auto scene_objects = scene->get_scene_objects();
		//	for (size_t i = 0; i < scene_objects.size(); i++)
		//	{
		//		if (RenderableComponent* renderable; scene_objects[i]->get_component(renderable))
		//		{
		//			if (WorldPositionComponent* world_pos; scene_objects[i]->get_component(world_pos))
		//			{
		//				glm::mat4 model = glm::mat4(1.0f);
		//				model = glm::translate(model, world_pos->position);
		//				render_requests.emplace_back(RenderRequest{ 0, 0, model, 0 ,0 });
		//			}
		//		}
		//	}
		//}
	}

}