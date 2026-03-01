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
#include <engine/utilities/entt_modified.h>
#include <engine/utilities/utilities.h>
#include <engine/core/worker_thread.h>
#include <engine/core/systems_context.h>
#include <engine/game/systems.h>

struct EditorRequestedAssets
{
	std::vector<hashed_string_64> materials;
	std::vector<hashed_string_64> meshes;
};


struct EditorEntity
{
	TransformDynamicComponent* transform = nullptr;
	RenderComponent* render = nullptr;
	PointLightComponent* point_light = nullptr;
	DirectionalLightComponent* directional_light = nullptr;
};

struct EngineContext
{
	EngineContext(std::shared_ptr<Scene> a_scene/*std::shared_ptr<MeshDataGenStorage> a_storage*/)
		: /*storage(a_storage), */ /* /*model_storage(std::make_shared<ModelGenStorage>()),*/ scene(a_scene), editor_camera("editor camera", 2560, 1440),
		input_manager(InputManager::instance())
	{
		num_logical_cores = std::thread::hardware_concurrency();
		num_game_threads = std::max(1, num_logical_cores - 2);
		num_general_threads = num_game_threads;
		PRINTLN("logical cores: {}", num_logical_cores);
		int num_asset_threads = 1;
		if (num_logical_cores <= 4)
		{
			num_asset_threads = 1;
		}
		else if (num_logical_cores <= 8)
		{
			//num_general_threads = 2;
			num_asset_threads = 2;
		}
		else if (num_logical_cores <= 12)
		{
			//num_general_threads = 3;
			num_asset_threads = 3;
		}
		else if (num_logical_cores <= 18)
		{
			//num_general_threads = 3;
			num_asset_threads = 4;
		}
		else
		{
			//num_general_threads = 4;
			num_asset_threads = 6;
		}
		asset_manager = std::make_shared<AssetManager>(num_asset_threads);
		worker_pool = std::make_shared<WorkerThreadPool>(num_game_threads);
		worker_pool->start();
		general_pool = std::make_shared<WorkerThreadPool>(num_general_threads);
		general_pool->start();
		systems_context = std::make_shared<SystemsContext>(scene->get_registry(), worker_pool, general_pool);
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
		render_contexts[std::size_t(write_pos)] = RenderContext(); // what's this? this the new one I think?
		cameras_render[std::size_t(write_pos)].clear();
	}
	bool run() { return !kill_all; }

	void pause_game_thread(const bool a_pause)
	{
		game_paused = a_pause;
	}

	void start_game_thread(const bool a_start)
	{
		if (!game_playing && a_start)
		{
			scene->save("./assets/temp/temp_scene.scn");

			scene->start_runtime();
			game_playing = true;
		}
		else if (!a_start && game_playing && scene->is_runtime())
		{
			game_playing = false;
			game_paused = false;
			requested_assets.materials.clear();
			requested_assets.meshes.clear();
			systems_context->reset();
			scene->stop_runtime();
			scene->load("./assets/temp/temp_scene.scn");
		}
	}

	void update_input()
	{
		input_manager.update_input();
	}

	void exit_game()
	{
		kill_all = true;
	}

	void reset_context()
	{
		systems_context->reset();
	}

	std::shared_ptr<AssetManager>& get_asset_manager()
	{
		return asset_manager;
	}

	uint64_t get_job_time() { return job_time++; }

	std::mutex mutex;
	std::condition_variable cv_frame_coordinator;
	std::condition_variable cv_threads;
	std::atomic<bool> write_pos = false;
	std::vector<RenderRequest> render_requests[2];
	RenderContext render_contexts[2];
	std::vector<CameraData> cameras_render[2];
	UIRenders UI_renders[2];
	//std::shared_ptr<MeshDataGenStorage> storage;
	//std::shared_ptr<ModelGenStorage> model_storage;
	std::shared_ptr<TextureLoader> texture_loader;
	std::shared_ptr<AssetManager> asset_manager;
	std::shared_ptr<Scene> scene;
	EditorRequestedAssets requested_assets;
	std::shared_ptr<WorkerThreadPool> worker_pool;
	std::shared_ptr<WorkerThreadPool> general_pool;
	std::shared_ptr<SystemsContext> systems_context;

	Camera editor_camera = Camera("editor camera", 2560, 1440);
	InputManager& input_manager;
	//MessageQueue<>
	bool kill_all = false;
	double delta_time = 0;
	bool game_thread = false;
	bool render_thread = false;
	bool next_frame = false;
	bool game_playing = false;
	bool spent_time = false;
	bool game_paused = false;
	bool physics_frame = false;
	std::atomic<uint64_t> job_time = 0;
	std::mutex glfw_mutex;

	int num_game_threads = 0;
	int num_logical_cores = 0;
	int num_general_threads = 0;
	
};

struct GameThread
{
	GameThread(std::shared_ptr<EngineContext> a_context, std::shared_ptr<Scene> a_scene)
		: engine_context(a_context), scene(a_scene){};

	void execute();
	void game_runtime();
	void editor_time();
private:
	std::shared_ptr<EngineContext> engine_context;
	std::shared_ptr<Scene> scene;

	size_t previous_total_render_requests = 10;
	size_t previous_unique_meshes = 10;
	size_t previous_unique_cameras = 2;

	HighResolutionTimer timer;
	HighResolutionTimer ind_timer;
	double movement = 0;
	double complex_movement = 0;
	double cameras_db = 0;
	double mesh_tex_jobs = 0;
	double pre_render_requests_time = 0;
	double renderrequests_time = 0;
	double game_thread_time = 0;
	double lighting_time = 0;

	MoveSystem move_system;
	TransformCalculationSystem transform_calculation_system;
	RenderRequestsSystem render_request_system;
	double accumilated_time = 0;
	// worker threads
};