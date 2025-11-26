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

//struct LockKey
//{
//	uint8_t key;
//	std::unique_lock<std::mutex> lock;
//	LockKey(uint8_t a_key, std::unique_lock<std::mutex>&& a_lock) : key(a_key), lock(std::move(a_lock)) {};
//};
//
//
//struct DataGenerationStorageBase
//{
//	virtual const std::type_info& get_data_type() = 0;
//	virtual const std::type_info& get_class_type() = 0;
//
//	uint32_t get_next_id() { std::lock_guard<std::mutex> guard(mutex); uint32_t result = next_id++; return result; };
//
//	uint32_t next_id = 0;
//	std::mutex mutex;
//	uint8_t key;
//};

// lock orders
// exit -> message
// data -> message
// for free roam read... lock mutex, if there are writers/waiting writers, wait cv_read, otherwise increment readers count -> unlock -> read -> lock -> decrement -> unlock if 1 writer notify cv_write
// readers lock mutex, increase readers count, wait cv_write if readers, -> write -> unlock -> notify all writers then readers
struct MeshDataGenStorage
{
	MeshDataGenStorage(std::shared_ptr<MessageQueue<LoadRequest>> a_load_request_messeges, std::shared_ptr<MessageQueue<VAORequest>> a_vao_request_messeges,
		uint8_t a_max_threads = 2) 
		: load_request_messages(a_load_request_messeges), vao_request_messages(a_vao_request_messeges), num_threads(a_max_threads)
	{
		mesh_datas.reserve(10);
		for (size_t i = 0; i < num_threads; i++)
		{
			threads.emplace_back(&MeshDataGenStorage::handle_load_requests, this);
		}
	};
	~MeshDataGenStorage()
	{
		{
			std::lock_guard<std::mutex> guard(exit_mutex);
			exit = true;
		}
		cv_threads.notify_all();
		for (std::thread& thread : threads)
		{
			if (thread.joinable())
			{
				thread.join();
			}
		}
	};
	
	// need to be able to get keydata from mesh_id
	// 
	// need to be able to give RenderRequest data from mesh_id
	RenderRequest get_render_request(entt::hashed_string& a_hashed_string)
	{
		std::lock_guard<std::mutex> guard(data_mutex);
		for (auto mesh_data : mesh_datas)
		{
			if (a_hashed_string == mesh_data.path_hashed)
			{
				return RenderRequest{mesh_data.VAO, ((GLsizei)mesh_data.indices.size()), glm::mat4(1.0f), 0, 0};
			}
		}
		return RenderRequest();
	}

	std::optional<VAORequest> get_vao_request()
	{
		return vao_request_messages->get_message();
	}

	bool load_request(const entt::hashed_string a_hashed_string)
	{
		std::unique_lock<std::mutex> lock(data_mutex);
		for (auto mesh_data : mesh_datas)
		{
			// good for like 2000-7000 different meshes supposedly... 
			// If I really need to, chop up string in two by every other char
			if (a_hashed_string == mesh_data.path_hashed)
			{
				PRINTLN("blocked by load request -- duplicate");
				if (mesh_data.bad_load && !mesh_data.started_load)
				{
					PRINTLN("attempting reload by bad load -- duplicate");
					std::string temp_string = a_hashed_string.data();
					load_request_messages->add_message(LoadRequest{ temp_string });
					std::lock_guard<std::mutex> thread_guard(thread_mutex);
					cv_threads.notify_one();
					return true;
				}
				return true;
			}
		}
		mesh_datas.emplace_back(MeshData());
		mesh_datas.back().path_hashed = a_hashed_string;
		std::string temp_string = a_hashed_string.data();
		lock.unlock();
		load_request_messages->add_message(LoadRequest{ temp_string});
		std::lock_guard<std::mutex> thread_guard(thread_mutex);
		cv_threads.notify_one();
		return true;
	}

	void load_requests(const std::vector<entt::hashed_string>& a_hashed_strings)
	{
		std::unique_lock<std::mutex> lock(data_mutex);
		std::vector<LoadRequest> load_requests;
		for (const entt::hashed_string& a_hashed_string : a_hashed_strings)
		{
			for (auto& mesh_data : mesh_datas)
			{
				// good for like 2000-7000 different meshes supposedly... 
				// If I really need to, chop up string in two by every other char ... or just do hashing myself, but annoying
				if (a_hashed_string == mesh_data.path_hashed)
				{
					continue;
				}
			}
			mesh_datas.emplace_back();
			mesh_datas.back().path_hashed = a_hashed_string;
			load_requests.emplace_back(a_hashed_string.data());
		}
		if (load_requests.size() > 0)
		{
			lock.unlock();
			load_request_messages->add_messages(load_requests);
			std::lock_guard<std::mutex> thread_guard(thread_mutex);
			cv_threads.notify_all();
		}
	}

	void handle_load_requests()
	{
		while (!exit)
		{
			if (auto message = load_request_messages->get_message()) // message is immedietly unlocked, is fine yo
			{
				PRINTLN("got message");
				entt::hashed_string check_hash(message.value().path.c_str());
				bool skip = false;
				{
					std::lock_guard<std::mutex> mesh_guard(data_mutex);
					for (auto& mesh_data : mesh_datas)
					{
						if (check_hash == mesh_data.path_hashed)
						{
							if (mesh_data.started_load == true)
								skip = true;
							else
								mesh_data.started_load = true;
							break;
						}
					}
				}

				if (!skip)
				{
					MeshData new_mesh_data = obj_parser.load_mesh_from_filepath(message.value().path); // problem is that it's not checking so it's not already loading it
					if (!new_mesh_data.bad_load)
					{
						new_mesh_data.path_hashed = entt::hashed_string(message.value().path.c_str());
						std::lock_guard<std::mutex> guard(data_mutex);
						bool added = false;
						for (auto& mesh_data : mesh_datas)
						{
							if (mesh_data.path_hashed == new_mesh_data.path_hashed)
							{
								PRINTLN("found entry");
								mesh_data = new_mesh_data;
								added = true;
								PRINTLN("adding entry into vao");
								vao_request_messages->add_message(VAORequest{ mesh_data.mesh_id, mesh_data.path_hashed }); // try this in thread tomorrow
								break;
							}
						}
						if (!added)
						{
							mesh_datas.push_back(new_mesh_data);
						}
					}
					else
					{
						std::lock_guard<std::mutex> guard(data_mutex);
						for (auto& mesh_data : mesh_datas)
						{
							if (mesh_data.path_hashed == new_mesh_data.path_hashed)
							{
								PRINTLN("found fail load entry");
								mesh_data.started_load = false; // maybe do like 10 attempts or so before giving up
								break;
							}
						}
					}
				}
				else PRINTLN("skipping mesh load");
				
			}
			{
				std::unique_lock lock(thread_mutex);
				cv_threads.wait(lock, [this] { return wait_check(); });
			}
			PRINTLN("obj loader woke up");
		}
	}

	bool wait_check() 
	{
		bool result = false;
		std::lock_guard<std::mutex> guard(exit_mutex);
		result |= exit;
		bool is_empty = !load_request_messages->is_empty();
		result |= is_empty;
		return result;
	}

	ObjParser obj_parser;
	uint8_t num_threads;
	bool exit = false;
	std::vector<std::thread> threads;

	// these must be set on initialization.
	std::shared_ptr<MessageQueue<LoadRequest>> load_request_messages;
	std::shared_ptr<MessageQueue<VAORequest>> vao_request_messages;

	std::condition_variable cv_threads;
	std::mutex data_mutex;
	std::mutex exit_mutex;
	std::mutex thread_mutex;

	// these need to sync up
	std::vector<MeshData> mesh_datas;
	std::map<entt::hashed_string, bool> string_loaded; // not gonna use
};

//template <typename TDataIn, typename TDataOut>
//struct DataGenerationStorage : DataGenerationStorageBase
//{
//
//	virtual uint32_t load_from_path(const std::string& a_path)
//	{
//		std::lock_guard<std::mutex> guard(mutex);
//		auto it = map_string_data.find(a_path);
//		if (it == map_string_data.end())
//			return handle_load_from_path(a_path);
//		else return find_id_from_index(it->second);
//	}
//
//	virtual std::optional<TDataOut> get_data_by_id(uint32_t a_id)
//	{
//		std::lock_guard<std::mutex> guard(mutex);
//		return retreive_data_by_id(a_id);
//	}
//
//	virtual void sort_data() = 0;
//	virtual std::optional<std::weak_ptr<TData>> get_data_by_id(const LockKey& a_lock, uint32_t a_id) = 0;
//
//	const std::type_info& get_data_type() override { return typeid(TDataIn); }
//	const std::type_info& get_class_type() override { return typeid(*this); }
//protected:
//	virtual uint32_t handle_load_from_path(const std::string& a_path) = 0;
//	virtual uint32_t find_id_from_index(size_t a_index) = 0;
//	virtual std::optional<TDataOut> retreive_data_by_id(uint32_t a_id) = 0;
//
//	std::vector<TDataIn> storage_data; // this is kinda terrible... the value is non-contigious
//	std::map<std::string, size_t index> map_string_data; // take in a string, and map it to the TData
//};
