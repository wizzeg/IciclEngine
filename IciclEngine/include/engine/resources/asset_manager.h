#pragma once
#include <cstdint>
#include <string>
#include <engine/renderer/render_info.h>
#include <engine/game/components.h>
#include "obj_parser.h"
#include <mutex>
#include <thread>
#include <functional>
#include <unordered_map>
#include <atomic>
#include <engine/resources/message_queue.h>
#include "job_info.h"

using AssetJob = std::variant<MeshDataJob, VAOLoadInfo, TextureDataJob, TextureGenInfo, ShaderDataJob, ValidateMatDependencies, ProgramLoadRequest, ProgramLoadInfo, MaterialDataJob, MaterialUniformJob>;


struct AssetStorage
{
	std::unordered_map<uint64_t, std::shared_ptr<MeshData>> mesh_map; // maybe should've been unique, don't want to change now
	std::mutex mesh_mutex;
	std::unordered_map<uint64_t, std::shared_ptr<TextureData>> tex_map;
	std::mutex tex_mutex;
	std::unordered_map<uint64_t, std::shared_ptr<MaterialData>> mat_map;
	std::mutex mat_mutex;
	std::unordered_map<uint64_t, std::shared_ptr<ShaderData>> shader_map;
	std::mutex shader_mutex;

	// when texture is loaded, update mat dependencies, if all dependencies are completed, add runtime_material
	std::unordered_map<uint64_t, std::vector<uint64_t>> mat_tex_dependencies; // tex -> vector of mat
	std::mutex dep_tex_mutex; // mat lock -> dep lock

	std::unordered_map<uint64_t, std::vector<uint64_t>> mat_shader_dependencies; // tex -> vector of mat
	std::mutex dep_shader_mutex; // mat lock -> dep lock

	std::vector<RuntimeMesh> runtime_meshes;
	std::mutex runtime_mesh_mutex;
	std::vector<RuntimeMaterial> runtime_materials;
	std::mutex runtime_mat_mutex;
};

struct AssetMessages
{
	MessageQueue<AssetJob> job_queue;
	MessageQueue<VAOLoadRequest> vao_queue;
	MessageQueue<ProgramLoadRequest> program_queue;
	MessageQueue<TexGenRequest> texgen_queue;
};

struct JobThread
{
	JobThread(){}
	~JobThread() { stop_thread(); }
	void set_id(uint8_t a_id) { id = a_id; }
	bool is_started() { return started; }
	bool is_initialized() { return initialized; }
	void initialize(std::mutex* a_thread_sync_mutex, std::condition_variable* a_thread_cv, bool* a_all_exit)
	{
		if (initialized) return;
		thread_sync_mutex = a_thread_sync_mutex;
		thread_cv = a_thread_cv;
		all_exit = a_all_exit;
		initialized = true;
	}
	bool start_thread()
	{
		if (!initialized) return false;
		started = true;
		this_exit = false;
		sleeping = false;
		thread = std::thread([this]() { job_loop(); });
		return true;
	}
	bool stop_thread()
	{
		if (started && thread.joinable())
		{
			{
				std::lock_guard<std::mutex> thread_guard(*thread_sync_mutex);
				this_exit = true;
				thread_cv->notify_all();
			}
			thread.join();
			started = false;
			sleeping = true;
			return true;
		}
		return false;
	}
	bool is_sleeping()
	{
		return sleeping;
	}
protected:
	virtual void job_loop() = 0;
	
	std::thread thread;
	std::mutex* thread_sync_mutex = nullptr;
	std::condition_variable* thread_cv = nullptr;
	bool* all_exit = nullptr;
	uint8_t id = 0;
	bool started = false;
	bool initialized = false;
	bool this_exit = false;
	bool sleeping = true;
};

struct AssetJobThread : JobThread
{
	AssetJobThread(AssetStorage& a_asset_storage, AssetMessages& a_asset_messages, uint8_t a_id) 
		: asset_storage(a_asset_storage), asset_messages(a_asset_messages)
	{
		id = a_id;
	}
protected:
	void job_loop() override;
	void process_load_mesh_job(MeshDataJob& a_job);
	void process_tex_job(TextureDataJob& a_job);
	void process_shader_job(ShaderDataJob& a_job);
	void process_mat_job(MaterialDataJob& a_job);
	void process_mesh_update(VAOLoadInfo& a_job);
	void process_tex_update(TextureGenInfo& a_job);
	void process_program_update(ProgramLoadInfo& a_job);
	void process_dependency(ValidateMatDependencies& a_job);
	void process_material_uniform(MaterialUniformJob& a_job);

	void insert_runtime_material(uint64_t a_mat_hash) {}; // to do use this one for clean up
	// todo generally try to generalize the processing
	AssetStorage& asset_storage;
	AssetMessages& asset_messages;
};

struct ThreadPool
{
	ThreadPool() {}
	~ThreadPool()
	{
		stop_threads();
		threads.clear();
	}
	void start_threads()
	{
		for (auto& thread : threads)
		{
			if (thread->is_started()) continue;
			if (!thread->is_initialized())
				static_cast<JobThread*>(thread.get())->initialize(&thread_sync_mutex, &thread_cv, &all_exit);
			thread->start_thread();
		}
	}
	template <typename TThread, typename ... Args>
	void add_thread(Args&&... args)
	{
		static_assert(std::is_base_of_v<JobThread, TThread>, "TThread must inherit from JobThread");
		auto job_thread = std::make_unique<TThread>(std::forward<Args>(args)...);
		job_thread->initialize(&thread_sync_mutex, &thread_cv, &all_exit);
		threads.emplace_back(std::move(job_thread));
		
	}
	void wake_threads()
	{
		{
			//std::unique_lock<std::mutex> thread_lock(thread_sync_mutex);
			thread_cv.notify_all();
		}
	}
	void wake_thread()
	{
		{
			//std::unique_lock<std::mutex> thread_lock(thread_sync_mutex);
			thread_cv.notify_one();
		}
	}
	void stop_threads()
	{
		if (!threads.empty())
		{
			{
				std::unique_lock<std::mutex> thread_lock(thread_sync_mutex);
				all_exit = true;
				thread_cv.notify_all();
			}
			for (auto& thread : threads)
			{
				thread->stop_thread();
			}
		}
	}
	bool all_exit = false;
	std::vector<std::unique_ptr<JobThread>> threads;
	std::mutex thread_sync_mutex;
	std::condition_variable thread_cv;
};



struct AssetManager
{
	AssetManager(uint8_t a_num_threads = 2)
	{
		for (uint8_t i = 0; i < a_num_threads; i++)
		{
			entt_thread_pool.add_thread<AssetJobThread>(asset_storage, asset_messages, i);
		}
		entt_thread_pool.start_threads();
	}
	~AssetManager()
	{
		entt_thread_pool.stop_threads();
	}

	void add_asset_job(AssetJob& a_job)
	{
		asset_messages.job_queue.add_message(a_job);
		entt_thread_pool.wake_thread();
	}
	void add_asset_jobs(std::vector<AssetJob>& a_jobs)
	{
		if (a_jobs.empty()) return;
		else if(a_jobs.size() == 1)
		{
			add_asset_job(a_jobs[0]);
		}
		else
		{
			asset_messages.job_queue.add_messages(a_jobs);
			entt_thread_pool.wake_threads();
		}

	}

	std::optional<VAOLoadRequest> get_vao_request() { return asset_messages.vao_queue.get_message(); }
	std::optional<ProgramLoadRequest> get_program_request() { return asset_messages.program_queue.get_message(); }
	std::optional<TexGenRequest> get_gen_request() { return asset_messages.texgen_queue.get_message(); }

	std::vector<RenderRequest> retrieve_render_requests(std::vector<PreRenderRequest>& a_pre_reqs);
	RenderContext construct_render_context(std::vector<PreRenderReq>& a_pre_reqs);

protected:
	AssetStorage asset_storage;
	AssetMessages asset_messages;
	ThreadPool entt_thread_pool;
};

