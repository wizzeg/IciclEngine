#pragma once
#include <mutex>
#include <functional>
#include <map>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <deque>
#include <optional>
#include <engine/renderer/renderer.h>
#include <barrier>
#include <latch>
#include <condition_variable>
#include <engine/editor/scene.h>

struct VAORequest
{
	uint32_t mesh_id;
};

template <typename T>
struct MessageQueue
{
	std::deque<T> messages;
	std::mutex mutex;

	void add_message(T& a_message)
	{
		std::lock_guard guard(mutex);
		messages.push_back(a_message);
	}


	std::optional<T> get_message()
	{
		std::lock_guard guard(mutex);
		if (messages.empty())
		{
			return std::nullopt;
		}
		T message = messages.front();
		messages.pop_front();
		return message;
	}
};

struct EngineContext
{
	void set_render_request(std::vector<RenderRequest>& a_render_requests)
	{
		render_requests[(std::size_t(write_pos))] = a_render_requests;
	}
	std::vector<RenderRequest>& get_render_requests()
	{
		return render_requests[(std::size_t(!write_pos))];
	}
	void swap_render_requests() { write_pos = !write_pos; }
	bool run() { return !kill_all; }
	std::mutex mutex;
	std::condition_variable cv_frame_coordinator;
	std::condition_variable cv_threads;
	bool write_pos = false;
	std::vector<RenderRequest> render_requests[2];
	std::atomic<bool> kill_all;
	bool game_thread = false;
	bool render_thread = false;
	bool next_frame = false;
	bool game_playing = false;
	
};

struct RenderContext
{
	RenderContext(Renderer& a_renderer, std::shared_ptr<MessageQueue<VAORequest>> a_vao_load_requests,
		std::shared_ptr<MessageQueue<VAORequest>> a_vao_delete_requests, std::weak_ptr<EngineContext> a_engine_context, 
		std::weak_ptr<ShaderProgram> a_shader_program)
		: renderer(a_renderer), vao_load_requests(a_vao_load_requests), vao_delete_requests(a_vao_delete_requests), 
		engine_context(a_engine_context) { renderer.temp_set_shader(a_shader_program); };

	std::vector<RenderRequest> render_requests;
	Renderer renderer;
	std::shared_ptr<MessageQueue<VAORequest>> vao_load_requests;
	std::shared_ptr<MessageQueue<VAORequest>> vao_delete_requests;
	std::weak_ptr<EngineContext> engine_context;
};

struct GameContext
{
	GameContext(std::weak_ptr<EngineContext> a_engine_context, std::weak_ptr<Scene> a_scene, MeshData& a_mesh) :
		engine_context(a_engine_context), scene(a_scene), mesh(a_mesh) {
	};
	std::weak_ptr<EngineContext> engine_context;
	std::weak_ptr<Scene> scene;
	MeshData mesh;
};





struct RenderThread
{

	RenderThread(std::shared_ptr<EngineContext> a_context, std::shared_ptr<MessageQueue<VAORequest>> a_vao_load_requests,
		std::shared_ptr<MessageQueue<VAORequest>> a_vao_delete_requests, std::weak_ptr<ShaderProgram> a_shader)
		: engine_context(a_context), vao_load_requests(a_vao_load_requests), vao_delete_requests(a_vao_delete_requests)
	{
		renderer.temp_set_shader(a_shader);
	};

	void execute();
	//void execute(std::weak_ptr<RenderContext> render_context);

private:
	std::shared_ptr<EngineContext> engine_context;
	std::vector<RenderRequest> render_requests;
	
	Renderer renderer;
	std::shared_ptr<MessageQueue<VAORequest>> vao_load_requests;
	std::shared_ptr<MessageQueue<VAORequest>> vao_delete_requests;
};

struct GameThread
{
	GameThread(std::shared_ptr<EngineContext> a_context, std::shared_ptr<Scene> a_scene, MeshData& a_mesh)
		: engine_context(a_context), scene(a_scene), mesh(a_mesh) {};

	void execute();
	void exdcute(std::weak_ptr<GameContext> a_game_context);

private:
	std::shared_ptr<EngineContext> engine_context;
	std::shared_ptr<Scene> scene;
	MeshData mesh;
};

struct EngineThread
{
	EngineThread(std::shared_ptr<EngineContext> a_context)
		: engine_context(a_context) { };

	void execute();

private:
	std::shared_ptr<EngineContext> engine_context;
};