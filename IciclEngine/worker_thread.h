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
#include <engine/renderer/glfw_context.h>
#include <engine/ui/imgui_manager.h>
#include <engine/ui/ui_manager.h>
#include <engine/game/camera.h>
#include <engine/game/input_manager.h>
#include <engine/renderer/texture_loader.h>
#include <engine/resources/asset_manager.h>

struct EngineContext
{
	EngineContext(/*std::shared_ptr<MeshDataGenStorage> a_storage*/) 
		: /*storage(a_storage), */ model_storage(std::make_shared<ModelGenStorage>()), editor_camera("editor camera", 1280, 960),
		input_manager(InputManager::get()), asset_manager(std::make_shared<AssetManager>())
	{
	};
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
		render_contexts[std::size_t(write_pos)] = RenderContext();
		cameras_render[std::size_t(write_pos)].clear();
	}
	bool run() { return !kill_all; }

	uint64_t get_job_time() { return job_time++; }

	std::mutex mutex;
	std::condition_variable cv_frame_coordinator;
	std::condition_variable cv_threads;
	std::atomic<bool> write_pos = false;
	std::vector<RenderRequest> render_requests[2];
	RenderContext render_contexts[2];
	std::vector<CameraData> cameras_render[2];
	//std::shared_ptr<MeshDataGenStorage> storage;
	std::shared_ptr<ModelGenStorage> model_storage;
	std::shared_ptr<TextureLoader> texture_loader;
	std::shared_ptr<AssetManager> asset_manager;

	Camera editor_camera = Camera("editor camera", 720, 480);
	InputManager& input_manager;
	//MessageQueue<>
	std::atomic<bool> kill_all;
	double delta_time = 0;
	bool game_thread = false;
	bool render_thread = false;
	bool next_frame = false;
	bool game_playing = false;
	bool spent_time = false;
	std::atomic<uint64_t> job_time = 0;
	std::mutex glfw_mutex;
	
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