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
		if (engine_context->game_playing)
		{
			auto& registry = scene->get_registry();

			for (auto [entity, name, worldpos] : registry.view<NameComponent, WorldPositionComponent>().each())
			{
				worldpos.position.x += 0.0005f;
			}

			for (auto [entity, mesh_component] : registry.view<MeshComponent>().each())
			{
				if (mesh_component.id == 0)
				{
					engine_context->storage->load_request(mesh_component.hashed_path);
					mesh_component.id = 1;
				}
			}

			//for (auto [entity, world_position] : registry.view<WorldPositionComponent>().each())
			//{
			//	glm::mat4 model = glm::mat4(1.0f);
			//	//model = glm::translate(model, world_position.position);
			//	render_requests.emplace_back(RenderRequest{ mesh.VAO, (GLsizei)mesh.indices.size(), model, (uint32_t)entity, (uint32_t)entity });
			//}
		}
		else
		{
			auto scene_objects = scene->get_scene_objects();
			for (size_t i = 0; i < scene_objects.size(); i++)
			{
				if (RenderableComponent* renderable; scene_objects[i]->get_component(renderable))
				{
					if (WorldPositionComponent* world_pos; scene_objects[i]->get_component(world_pos))
					{
						glm::mat4 model = glm::mat4(1.0f);
						model = glm::translate(model, world_pos->position);
						render_requests.emplace_back(RenderRequest{ 0, 0, model, 0 ,0 });
					}
				}
			}
		}
		// do game stuff
		// write render requests
	}

}