#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <random>
#include <engine/utilities/macros.h>
#include <engine/renderer/render_info.h>
#include <engine/resources/obj_parser.h>
#include <engine/resources/message_queue.h>
#include <condition_variable>
#include <glad/glad.h>
#include <string>
#include <map>
#include <variant>
#include <engine/resources/job_info.h>
#include <engine/resources/model_loader.h>

struct ModelGenStorage;
struct GenStorageThreadPool;

using LoadJob = std::variant<MeshDataJob, VAOLoadInfo, TextureDataJob, TextureGenInfo>;

template <typename TReturn, typename TRequest>
struct JobRequestTyped
{
	virtual std::optional<TReturn> return_request(const TRequest& a_request) = 0;
	virtual std::optional<std::vector<TReturn>> return_requests(std::vector<TRequest>& a_requests, bool sorted = false) = 0;
};
template <typename TReturn>
struct JobRequest
{
	virtual std::optional<TReturn> return_request() = 0;
	virtual std::optional<std::vector<TReturn>> return_requests() = 0;
};

struct RenderRequestReturner : JobRequestTyped<RenderRequest, PreRenderRequest>
{
	RenderRequestReturner(ModelGenStorage& a_gen_storage);
	std::optional<RenderRequest> return_request(const PreRenderRequest& a_request) override;
	std::optional<std::vector<RenderRequest>> return_requests(std::vector<PreRenderRequest>& a_request, bool sorted = false) override;
protected:
	ModelGenStorage& renderreqret_gen_storage;
};

struct VAOLoadRequester : JobRequest<VAOLoadRequest>
{
	VAOLoadRequester(ModelGenStorage& a_gen_storage);
	std::optional<VAOLoadRequest> return_request() override;
	std::optional<std::vector<VAOLoadRequest>> return_requests() override;
protected:
	ModelGenStorage& vaoload_gen_storage;
};

struct TexGenRequester : JobRequest<TexGenRequest> // I could make these inherit a MessageQueue... but eh, not really
{
	TexGenRequester(ModelGenStorage& a_gen_storage);
	std::optional<TexGenRequest> return_request() override;
	std::optional<std::vector<TexGenRequest>> return_requests() override;
protected:
	ModelGenStorage& texgen_gen_storage;
};



template <typename TJob>
struct JobProcessor  // this only processes a given job
{
	virtual void process_job(TJob& a_job) = 0;
};

struct MeshJobProcessor : JobProcessor<MeshDataJob>
{
	MeshJobProcessor(ModelGenStorage& a_gen_storage);
	void process_job(MeshDataJob& a_job) override;
	//void generate_render_requests(RenderRequest& a_render_request);
protected:
	void sort_data(std::unique_lock<std::mutex>& a_lock); /// make sure you have locked it first
	ObjParser obj_parser;
	ModelGenStorage& meshjob_gen_storage;
};

struct VAOInfoProcessor : JobProcessor<VAOLoadInfo>
{
	VAOInfoProcessor(ModelGenStorage& a_gen_storage);
	void process_job(VAOLoadInfo& a_job) override;
protected:
	void sort_data(std::unique_lock<std::mutex>& a_lock); /// make sure you have locked it first
	ModelGenStorage& vaoinfo_gen_storage;
};

struct TextureDataProcessor : JobProcessor<TextureDataJob>
{
	TextureDataProcessor(ModelGenStorage& a_gen_storage);
	void process_job(TextureDataJob& a_job) override;
protected:
	void sort_data(std::unique_lock<std::mutex>& a_lock); /// make sure you have locked it first
	ModelGenStorage& texturedata_gen_storage;
};


struct TexGenInfoProcessor : JobProcessor<TextureGenInfo>
{
	TexGenInfoProcessor(ModelGenStorage& a_gen_storage);
	void process_job(TextureGenInfo& a_job) override;
protected:
	void sort_data(std::unique_lock<std::mutex>& a_lock); /// make sure you have locked it first
	ModelGenStorage& texgeninfo_gen_storage;
};

struct GenStorageThreadPool // this is abstract, must be inherited, this one holds jobs and adds jobs
{
	GenStorageThreadPool(size_t num_threads = 2);
	~GenStorageThreadPool();
	void start_threads(); // allowing to pass in a function and Args... args here could remove some coupling of modelgenstorage and processors
	void add_job(LoadJob& a_job);
	void add_jobs(std::vector<LoadJob>& a_jobs);
	void clear_jobs();
	virtual const std::type_info& get_cast_type() { return typeid(GenStorageThreadPool); };
protected:
	virtual void worker_loop(size_t a_thread_id) = 0;
	std::shared_ptr<MessageQueue<LoadJob>> job_messages;
	std::mutex thread_mutex;
	bool exit = false;
	std::condition_variable cv_new_job;
	bool start = false;
	std::condition_variable cv_start_threads;
	std::vector<std::thread> threads;
	size_t num_threads;
};

struct ModelGenStorage : GenStorageThreadPool // this will have more job processors, but only one genstoragethreadpool
{
	ModelGenStorage(size_t num_threads = 2); // this mf is EXTREMELY tightly coupled to all the other things, soo... doing any composition/polymorphism here really didn't give any benefits
	const std::type_info& get_cast_type() override;
	friend struct MeshJobProcessor;
	friend struct VAOInfoProcessor;
	friend struct RenderRequestReturner;
	friend struct VAOLoadRequester;
	friend struct TextureDataProcessor;
	friend struct TexGenRequester;
	friend struct TexGenInfoProcessor;
protected:
	void worker_loop(size_t a_thread_id) override; 
	bool worker_wait_condition();
	std::mutex mesh_mutex;
	std::vector<MeshData> mesh_datas;
	std::mutex texture_mutex;
	std::vector<TextureData> texuture_datas;
	MessageQueue<VAOLoadRequest> vaoload_requests;
	MessageQueue<TexGenRequest> texgen_requests;
	MeshJobProcessor mesh_job_processor;
	VAOInfoProcessor vaoinfo_job_processor;
	TextureDataProcessor tex_job_processor; /// need one texgeninfo_job_processor too...
	TexGenInfoProcessor tex_geninfo_processor;
	ModelLoader model_loader;
public:
	RenderRequestReturner render_request_returner;
	VAOLoadRequester vaoload_returner;
	TexGenRequester texgen_returner;
};

/////////////////////////////////////////////////////////////////////////////////////
// OLD
//struct MeshDataGenStorage // this now needs to be able to handle texture load requests.... How can I structure this?
//{
//	MeshDataGenStorage(uint8_t a_max_threads = 2) : num_threads(a_max_threads)
//	{
//		load_request_messages = std::make_shared<MessageQueue<LoadRequest>>();
//		vao_request_messages = std::make_shared<MessageQueue<VAOLoadRequest>>();
//		vao_update_messages = std::make_shared<MessageQueue<VAOLoadInfo>>();
//		mesh_datas.reserve(10);
//		for (size_t i = 0; i < num_threads; i++)
//		{
//			threads.emplace_back(&MeshDataGenStorage::handle_load_requests, this, i);
//		}
//	};
//	~MeshDataGenStorage()
//	{
//		{
//			std::lock_guard<std::mutex> guard(exit_mutex);
//			exit = true;
//		}
//		cv_threads.notify_all();
//		for (std::thread& thread : threads)
//		{
//			if (thread.joinable())
//			{
//				thread.join();
//			}
//		}
//	};
//
//	std::optional<VAOLoadRequest> get_vao_request()
//	{
//		return vao_request_messages->get_message();
//	}
//
//	void update_vao_info(const VAOLoadInfo& a_vao_load_info)
//	{
//		vao_update_messages->add_message(a_vao_load_info);
//		cv_threads.notify_one();
//	}
//	
//	// need to be able to get keydata from mesh_id
//	// 
//	// need to be able to give RenderRequest data from mesh_id
//	RenderRequest get_render_request(hashed_string_64& a_hashed_string)
//	{
//		std::lock_guard<std::mutex> guard(data_mutex);
//		for (auto mesh_data : mesh_datas)
//		{
//			if (a_hashed_string == mesh_data.path_hashed)
//			{
//				return RenderRequest{ a_hashed_string, mesh_data.VAO, ((GLsizei)mesh_data.indices.size()), glm::mat4(1.0f), 0, 0};
//			}
//		}
//		return RenderRequest();
//	}
//
//	// perhaps make this into unique ptr, or raw pointer, skip copy
//	std::vector<RenderRequest> get_render_request(std::vector<hashed_string_64>& a_hashed_string)  /// RELEIS ON SORTED VECTOR
//	{
//		std::vector<RenderRequest> render_requests;
//		size_t mesh_index = 0;
//		size_t start_index = 0;
//		bool none_found = true;
//		{
//			std::lock_guard<std::mutex> guard(data_mutex);
//			for (auto& hashed_string : a_hashed_string) // O(n) I think, not too bad ... especially if I cache them later
//			{
//				start_index = mesh_index;
//				none_found = true;
//				for (; mesh_index < mesh_datas.size(); mesh_index++)
//				{
//					if (hashed_string == mesh_datas[mesh_index].path_hashed)
//					{
//						RenderRequest request(hashed_string, mesh_datas[mesh_index].VAO, mesh_datas[mesh_index].indices.size(), glm::mat4(1), 0, 0);
//						render_requests.emplace_back(request);
//						mesh_index++;
//						none_found = false;
//						break;
//					}
//				}
//				if (none_found)
//				{
//					mesh_index = start_index;
//				}
//			}
//		}
//		return render_requests;
//	}
//
//
//
//	bool load_request(const hashed_string_64 a_hashed_string)
//	{
//		std::unique_lock<std::mutex> lock(data_mutex);
//		for (auto mesh_data : mesh_datas)
//		{
//			// good for like 2000-7000 different meshes supposedly... 
//			// If I really need to, chop up string in two by every other char
//			if (a_hashed_string == mesh_data.path_hashed)
//			{
//				PRINTLN("blocked by load request -- duplicate");
//				if (mesh_data.bad_load && !mesh_data.started_load)
//				{
//					PRINTLN("attempting reload by bad load -- duplicate");
//					std::string temp_string = a_hashed_string.string;
//					load_request_messages->add_message(LoadRequest{ temp_string });
//					std::lock_guard<std::mutex> thread_guard(thread_mutex);
//					cv_threads.notify_one();
//					return true;
//				}
//				return true;
//			}
//		}
//		mesh_datas.emplace_back(MeshData());
//		mesh_datas.back().path_hashed = a_hashed_string;
//		std::sort(mesh_datas.begin(), mesh_datas.end(), [](const MeshData& mesh_a, const MeshData& mesh_b)
//			{
//				return mesh_a.path_hashed < mesh_b.path_hashed;
//			});
//		std::string temp_string = a_hashed_string.string;
//		lock.unlock();
//		load_request_messages->add_message(LoadRequest{ temp_string});
//		std::lock_guard<std::mutex> thread_guard(thread_mutex);
//		cv_threads.notify_one();
//		return true;
//	}
//
//	void load_requests(const std::vector<hashed_string_64>& a_hashed_strings) // should do sorted here too for O(n) ... but I'll probably remove component anyway
//	{
//		std::unique_lock<std::mutex> lock(data_mutex);
//		std::vector<LoadRequest> load_requests;
//		bool exists = false;
//		for (const hashed_string_64& a_hashed_string : a_hashed_strings) // O(n) ... so sorting and this will be fine. I may try to do some mesh copying on entities later
//		{
//			for (auto& mesh_data : mesh_datas)
//			{
//				// good for like 2000-7000 different meshes supposedly... 
//				// If I really need to, chop up string in two by every other char ... or just do hashing myself, but annoying
//				if (a_hashed_string == mesh_data.path_hashed)
//				{
//					exists = true;
//					break;
//				}
//			}
//			if (!exists)
//			{
//				mesh_datas.emplace_back();
//				mesh_datas.back().path_hashed = a_hashed_string;
//				load_requests.emplace_back(a_hashed_string.string);
//
//			}
//			exists = false;
//		}
//		if (load_requests.size() > 0)
//		{
//			std::sort(mesh_datas.begin(), mesh_datas.end(), [](const MeshData& mesh_a, const MeshData& mesh_b)
//				{
//					return mesh_a.path_hashed < mesh_b.path_hashed;
//				});
//			lock.unlock();
//			load_request_messages->add_messages(load_requests);
//			std::lock_guard<std::mutex> thread_guard(thread_mutex);
//			for (size_t i = 0; i < load_requests.size(); i++)
//			{
//				cv_threads.notify_one();
//			}
//			
//		}
//	}
//
//	void handle_load_requests(size_t a_thread_id = 0)
//	{
//		while (!exit)
//		{
//			if (auto message = vao_update_messages->get_message())
//			{
//				PRINTLN("Recieved MeshData vao update request (thread {})", a_thread_id);
//				VAOLoadInfo& vao_info = message.value();
//				if (vao_info.vao_loaded)
//				{
//					std::lock_guard<std::mutex> mesh_guard(data_mutex);
//					for (auto& mesh_data : mesh_datas)
//					{
//						if (mesh_data.path_hashed == vao_info.hashed_path)
//						{
//							mesh_data.VAO_loaded = vao_info.vao_loaded;
//							mesh_data.VAO = vao_info.vao;
//							mesh_data.VBOs = vao_info.VBOs; // hm, not great copy, but it's like max 16 gluints
//							mesh_data.EBO = vao_info.ebo;
//							break;
//						}
//					}
//				}
//				else
//				{
//					// I dunno, just drop it I guess
//				}
//
//			}
//
//			if (auto message = load_request_messages->get_message()) // message is immedietly unlocked, is fine yo
//			{
//				PRINTLN("Recieved mesh obj load request (thread {})", a_thread_id);
//				hashed_string_64 check_hash(message.value().path.c_str());
//				bool skip = false;
//				{
//					std::lock_guard<std::mutex> mesh_guard(data_mutex);
//					for (auto& mesh_data : mesh_datas)
//					{
//						if (check_hash == mesh_data.path_hashed)
//						{
//							if (mesh_data.started_load == true)
//								skip = true;
//							else
//								mesh_data.started_load = true;
//							break;
//						}
//					}
//				}
//
//				if (!skip)
//				{
//					MeshData new_mesh_data = obj_parser.load_mesh_from_filepath(message.value().path); // problem is that it's not checking so it's not already loading it
//					if (!new_mesh_data.bad_load)
//					{
//						new_mesh_data.path_hashed = hashed_string_64(message.value().path.c_str());
//						std::lock_guard<std::mutex> guard(data_mutex);
//						bool added = false;
//						for (auto& mesh_data : mesh_datas)
//						{
//							if (mesh_data.path_hashed == new_mesh_data.path_hashed)
//							{
//								PRINTLN("found MeshData entry (thread {})", a_thread_id);
//								mesh_data = new_mesh_data;
//								added = true;
//								PRINTLN("Adding VAO requestmessage (thread {})", a_thread_id);
//								vao_request_messages->add_message(VAOLoadRequest{ mesh_data });
//								break;
//							}
//						}
//						if (!added)
//						{
//							mesh_datas.push_back(new_mesh_data);
//							std::sort(mesh_datas.begin(), mesh_datas.end(), [](const MeshData& mesh_a, const MeshData& mesh_b)
//								{
//									return mesh_a.path_hashed < mesh_b.path_hashed;
//								});
//						}
//					}
//					else
//					{
//						std::lock_guard<std::mutex> guard(data_mutex);
//						for (auto& mesh_data : mesh_datas)
//						{
//							if (mesh_data.path_hashed == new_mesh_data.path_hashed)
//							{
//								PRINTLN("found fail load entry");
//								mesh_data.started_load = false; // maybe do like 10 attempts or so before giving up
//								break;
//							}
//						}
//					}
//				}
//				else PRINTLN("skipping mesh load");
//				
//			}
//			{
//				std::unique_lock lock(thread_mutex);
//				cv_threads.wait(lock, [this] { return wait_check(); });
//			}
//			PRINTLN("MeshDataGenStorage thread({}) woke up", a_thread_id);
//		}
//	}
//
//	bool wait_check() 
//	{
//		bool result = false;
//		std::lock_guard<std::mutex> guard(exit_mutex);
//		result |= exit;
//		bool is_empty = !load_request_messages->is_empty();
//		result |= is_empty;
//		is_empty = !vao_update_messages->is_empty();
//		result |= is_empty;
//		return result;
//	}
//
//	ObjParser obj_parser;
//	uint8_t num_threads;
//	bool exit = false;
//	std::vector<std::thread> threads;
//
//	// these must be set on initialization.
//	std::shared_ptr<MessageQueue<LoadRequest>> load_request_messages;
//	std::shared_ptr<MessageQueue<VAOLoadRequest>> vao_request_messages;
//	std::shared_ptr<MessageQueue<VAOLoadInfo>> vao_update_messages;
//
//	std::condition_variable cv_threads;
//	std::mutex data_mutex;
//	std::mutex exit_mutex;
//	std::mutex thread_mutex;
//
//	// these need to sync up
//	std::vector<MeshData> mesh_datas; 
//	std::map<hashed_string_64, size_t> string_loaded; // should use for faster look up perhaps hashsed_string to index -- probably slower
//};
//
