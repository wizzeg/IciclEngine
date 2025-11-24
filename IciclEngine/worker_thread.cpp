#include "worker_thread.h"
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <engine/editor/scene_object.h>
#include <engine/renderer/render_info.h>
#include <glad/glad.h>
#include <glfw/glfw3.h>

void RenderThread::execute()
{
	int runs = 0;
	while (engine_context.get()->run())
	{
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//glClearColor(0.1f, 0.3f, 0.2f, 1.0f);
		auto& render_requests = engine_context.get()->render_requests[std::size_t(!engine_context.get()->write_pos)];
		//for (size_t i = 0; i < render_requests.size(); i++)
		//{
		//	renderer.temp_render(render_requests[i]);
		//}
		//glfwSwapBuffers(window);

		if (auto message = vao_load_requests.get()->get_message())
		{
			PRINTLN("found a message");
			// do load to vao
		}
		if (auto message = vao_delete_requests.get()->get_message())
		{
			// do delete vao
		}
		PRINTLN("render thread going to sleep: {}", runs++);
		std::unique_lock<std::mutex> lock(engine_context.get()->mutex);
		engine_context.get()->render_thread = false;
		engine_context.get()->cv_frame_coordinator.notify_all();
		engine_context.get()->cv_threads.wait(lock, [this] { return engine_context.get()->render_thread; });
		//engine_context.get()->cv_threads.wait(lock, [this] { return true; });
		//{
		//	std::lock_guard<std::mutex> lock(engine_context.get()->mutex);
		//	engine_context.get()->next_frame = false;
		//}
		PRINTLN("render thread woke up");
	}
}

//
//void RenderThread::execute(std::weak_ptr<RenderContext> a_render_context)
//{
//	while (engine_context.get()->run())
//	{
//		if (auto render_context = a_render_context.lock())
//		{
//			if (auto engine_context = render_context->engine_context.lock())
//			{
//				auto& render_requests = engine_context->render_requests[std::size_t(!engine_context.get()->write_pos)];
//				for (size_t i = 0; i < render_requests.size(); i++)
//				{
//					renderer.temp_render(render_requests[i]);
//				}
//
//				if (auto message = vao_load_requests.get()->get_message())
//				{
//					// do load to vao
//				}
//				if (auto message = vao_delete_requests.get()->get_message())
//				{
//					// do delete vao
//				}
//			}
//			else goto exit;
//		}
//		else 
//		{
//		exit:
//			PRINTLN("render thread lost render context, shutting down thread");
//			break;
//		}
//
//		PRINTLN("render thread going to sleep");
//		std::unique_lock<std::mutex> lock(engine_context.get()->mutex);
//		engine_context.get()->render_thread = true;
//		engine_context.get()->cv_frame_coordinator.notify_all();
//		engine_context.get()->cv_threads.wait(lock, [this] { return engine_context.get()->next_frame; });
//		{
//			std::unique_lock<std::mutex> lock(engine_context.get()->mutex);
//			engine_context.get()->next_frame = false;
//		}
//		PRINTLN("render thread woke up");
//	}
//}

void EngineThread::execute()
{
	int num_frames = 100000;
	while (num_frames-- > 0)
	{
		{
			std::unique_lock<std::mutex> lock(engine_context.get()->mutex);
			engine_context.get()->cv_frame_coordinator.wait(lock, [this] { return (engine_context.get()->render_thread == false && engine_context.get()->game_thread == false); });
		}

		{
			std::lock_guard<std::mutex> guard(engine_context.get()->mutex);
			engine_context.get()->swap_render_requests();
			engine_context.get()->render_thread = true;
			engine_context.get()->game_thread = true;
		}
		engine_context.get()->cv_threads.notify_all();
	}
}

void GameThread::execute()
{
	int runs = 0;
	while (engine_context.get()->run())
	{
		std::vector<RenderRequest> render_requests;
		if (engine_context.get()->game_playing)
		{
			auto& registry = scene.get()->get_registry();

			for (auto [entity, name, worldpos] : registry.view<NameComponent, WorldPositionComponent>().each())
			{
				worldpos.position.x += 0.0005f;
			}

			for (auto [entity, renderable, world_position] : registry.view<RenderableComponent, WorldPositionComponent>().each())
			{
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, world_position.position);
				render_requests.emplace_back(RenderRequest{ mesh.VAO, (GLsizei)mesh.indices.size(), model, 0 ,0 });
			}
		}
		else
		{
			auto scene_objects = scene.get()->get_scene_objects();
			for (size_t i = 0; i < scene_objects.size(); i++)
			{
				if (RenderableComponent* renderable; scene_objects[i].get()->get_component(renderable))
				{
					if (WorldPositionComponent* world_pos; scene_objects[i].get()->get_component(world_pos))
					{
						glm::mat4 model = glm::mat4(1.0f);
						model = glm::translate(model, world_pos->position);
						render_requests.emplace_back(RenderRequest{ mesh.VAO, (GLsizei)mesh.indices.size(), model, 0 ,0 });
					}
				}
			}
		}
		// do game stuff
		// write render requests

		PRINTLN("game thread going to sleep: {}", runs++);
		std::unique_lock<std::mutex> lock(engine_context.get()->mutex);
		engine_context.get()->game_thread = false;
		engine_context.get()->cv_frame_coordinator.notify_all();
		engine_context.get()->cv_threads.wait(lock, [this] { return engine_context.get()->game_thread; });
		//engine_context.get()->cv_threads.wait(lock, [this] { return true; });
		//{
		//	std::lock_guard<std::mutex> lock(engine_context.get()->mutex);
		//	engine_context.get()->next_frame = false;
		//}
		PRINTLN("game thread woke up");
	}

}