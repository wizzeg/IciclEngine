#pragma once
#include <mutex>
#include <functional>
#include <map>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <engine/resources/message_queue.h>
#include <optional>
#include <engine/renderer/renderer.h>
#include <barrier>
#include <latch>
#include <condition_variable>
#include <engine/editor/scene.h>
#include <engine/resources/data_storage.h>

#include <imgui-docking/imgui.h>
#include <imgui-docking/imgui_impl_glfw.h>
#include <imgui-docking/imgui_impl_opengl3.h>
#include "glfw_context.h"
#include "imgui_manager.h"
#include <engine/ui/ui_manager.h>
#include "camera.h"
#include "input_manager.h"

struct EngineContext
{
	EngineContext(std::shared_ptr<MeshDataGenStorage> a_storage) : storage(a_storage), model_storage(std::make_shared<ModelGenStorage>()), editor_camera("editor camera", 1280, 960), input_manager(InputManager::get()) {};
	void set_render_request(std::vector<RenderRequest>& a_render_requests)
	{
		render_requests[(std::size_t(write_pos))] = a_render_requests;
	}
	std::vector<RenderRequest>& get_render_requests()
	{
		return render_requests[(std::size_t(!write_pos))];
	}
	std::vector<CameraData>& get_camera_render()
	{
		return cameras_render[(std::size_t(!write_pos))];
	}
	void swap_render_requests()
	{
		write_pos = !write_pos;
		render_requests[std::size_t(write_pos)].clear();
		cameras_render[std::size_t(write_pos)].clear();
	}
	bool run() { return !kill_all; }
	std::mutex mutex;
	std::condition_variable cv_frame_coordinator;
	std::condition_variable cv_threads;
	std::atomic<bool> write_pos = false;
	std::vector<RenderRequest> render_requests[2];
	std::vector<CameraData> cameras_render[2];
	std::shared_ptr<MeshDataGenStorage> storage;
	std::shared_ptr<ModelGenStorage> model_storage;

	Camera editor_camera = Camera("editor camera", 720, 480);
	InputManager& input_manager;
	//MessageQueue<>
	std::atomic<bool> kill_all;
	double delta_time = 0;
	bool game_thread = false;
	bool render_thread = false;
	bool next_frame = false;
	bool game_playing = false;
	std::mutex glfw_mutex;
	
};

template <typename TMessage>
struct MessageThreadContext
{
	std::mutex mutex;
	std::condition_variable cv_message_queue;
	MessageQueue<TMessage> message_queue;
	bool exit = false;
};

struct MeshLoadThreadContext : MessageThreadContext<LoadRequest>
{
	//some asset manager
};


struct RenderThread
{

	RenderThread(std::shared_ptr<EngineContext> a_context, ShaderProgram& a_default_shader, std::weak_ptr<GLFWContext> a_gl_context)
		: engine_context(a_context), default_shader(a_default_shader), gl_context(a_gl_context)
	{
	};

	void execute();

private:
	std::shared_ptr<EngineContext> engine_context;
	Renderer renderer;
	ShaderProgram default_shader;
	std::weak_ptr<GLFWContext> gl_context;
};

struct GameThread
{
	GameThread(std::shared_ptr<EngineContext> a_context, std::shared_ptr<Scene> a_scene)
		: engine_context(a_context), scene(a_scene){};

	void execute();
private:
	std::shared_ptr<EngineContext> engine_context;
	std::shared_ptr<Scene> scene;

	// worker threads
};

struct EngineThread
{
	EngineThread(std::shared_ptr<EngineContext> a_context, std::weak_ptr<ImGuiManager> a_imgui_manager, std::weak_ptr<UIManager> a_ui_manager)
		: engine_context(a_context), imgui_manager(a_imgui_manager), ui_manager(a_ui_manager)
	{
	};

	void execute();

private:
	std::shared_ptr<EngineContext> engine_context;
	std::weak_ptr<ImGuiManager> imgui_manager;
	std::weak_ptr<UIManager> ui_manager;
};

template <typename TMessage>
struct MessageQueueThread
{
	MessageQueueThread(std::shared_ptr<MessageThreadContext<TMessage>> a_pool_context) 
		: pool_context(a_pool_context) {};
	void execute()
	{
		while (!pool_context->exit)
		{
			while (auto message = pool_context->message_queue.get_message())
			{
				handle_message(message.value());
			}
			{
				std::unique_lock<std::mutex> lock(pool_context->mutex);
				pool_context->cv_message_queue.wait(lock, [this] { return pool_context->message_queue.is_empty(); });
			}
		}
	}
	virtual void handle_message(TMessage& a_message) {};
	std::shared_ptr<MessageThreadContext<TMessage>> pool_context;
};

struct RenderRequestThread : MessageQueueThread<LoadRequest>
{
	RenderRequestThread(std::shared_ptr<MeshLoadThreadContext> a_pool_context) 
		: MessageQueueThread<LoadRequest>(static_cast<std::shared_ptr<MessageThreadContext<LoadRequest>>>(a_pool_context)) {}
	void handle_message(RenderRequest& a_message)
	{

	}
};



//struct RenderThread
//{
//
//	RenderThread(std::shared_ptr<EngineContext> a_context) : engine_context(a_context)
//	{
//		window = glfwGetCurrentContext();
//		if (!window)
//		{
//			PRINTLN("Failed getting WINDOW for RENDER THREAD");
//		}
//		// then I need to create the standard shader program
//		default_shader = std::make_unique<ShaderProgram>("./assets/shaders/vertex/vert.glsl", "./assets/shaders/fragment/frag.glsl");
//	};
//
//	void execute();
//
//private:
//	std::shared_ptr<EngineContext> engine_context;
//	Renderer renderer;
//	GLFWwindow* window;
//	std::unique_ptr<ShaderProgram> default_shader;
//};