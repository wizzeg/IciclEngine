#pragma once

#include <engine/core/worker_thread.h>
#include <engine/editor/scene.h>
#include <engine/utilities/entt_modified.h>
#include <functional>
#include <algorithm>
#include <typeindex>
#include <tuple>
#include <map>
#include <any>
#include <mutex>
#include <engine/utilities/macros.h>
#include <condition_variable>

template<typename...T> struct WithWrite {};
template<typename...T> struct WithRead {};
template<typename...T> struct Without {};

struct SystemsContextDependencies
{
	std::vector<std::type_index> reading_components;
	std::vector<std::type_index> writing_components;
	std::mutex components_mutex;

	template<typename... Reads, typename... Writes>
	bool add(WithRead<Reads...>, WithWrite<Writes...>, bool check_dependencies = true)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			if (check_dependencies)
				if (dependency_collision({ typeid(Reads)... }, { typeid(Writes)... }))
					return true;
			(add_write_dependency<Writes>(),...);
			(add_read_dependency<Reads>(),...);
		}
		return false;
	}

	template<typename... Reads>
	bool add(WithRead<Reads...>, bool check_dependencies = true)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			if (check_dependencies)
				if (dependency_collision({ typeid(Reads)... }, { }))
					return true;
			(add_read_dependency<Reads>(),...);
		}
		return false;
	}

	template<typename... Writes>
	bool add(WithWrite<Writes...>, bool check_dependencies = true)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			if (check_dependencies)
				if (dependency_collision({ }, { typeid(Writes)... }))
					return true;
			(add_write_dependency<Writes>(),...);
		}
		return false;
	}

	template<typename... Reads, typename... Writes>
	void remove(WithRead<Reads...>, WithWrite<Writes...>)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			(remove_write_dependency<Writes>(), ...);
			(remove_read_dependency<Reads>(), ...);
		}
	}

	template<typename... Reads>
	void remove(WithRead<Reads...>)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			(remove_read_dependency<Reads>(), ...);
		}
	}

	template<typename... Writes>
	void remove(WithWrite<Writes...>)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			(remove_write_dependency<Writes>(), ...);
		}
	}
protected:
	bool dependency_collision(std::vector<std::type_index>&& a_reads, std::vector<std::type_index>&& a_writes) // have to actually check if component that is going to be added collides
	{
		if (a_reads.size() > 0)
		{
			size_t write_index = 0;
			std::sort(a_reads.begin(), a_reads.end(), std::less<std::type_index>{});
			for (auto& read : a_reads)
			{
				for (; write_index < writing_components.size(); ++write_index)
				{
					auto& write = writing_components[write_index];
					if (read == write)
					{
						return true;
					}
					else if (write > read)
					{
						break;
					}
				}
			}
		}

		if (a_writes.size() > 0)
		{
			size_t read_index = 0;
			std::sort(a_writes.begin(), a_writes.end(), std::less<std::type_index>{});
			for (auto& write : a_writes)
			{
				for (; read_index < reading_components.size(); ++read_index)
				{
					auto& read = reading_components[read_index];
					if (write == read)
					{
						return true;
					}
					else if (read > write)
					{
						break;
					}
				}
			}
			size_t write_index = 0;
			for (auto& write : a_writes)
			{
				for (; write_index < writing_components.size(); ++write_index)
				{
					auto& comp_write = writing_components[write_index];
					if (write == comp_write)
					{
						return true;
					}
					else if (comp_write > write)
					{
						break;
					}
				}
			}
		}
		return false;
	}

	template<typename Read>
	void add_read_dependency()
	{
		std::type_index type = typeid(Read);
		size_t insertion_index = 0;
		for (; insertion_index < reading_components.size(); ++insertion_index)
		{
			auto& read_comp = reading_components[insertion_index];
			if (type < read_comp)
			{
				break;
			}
		}
		reading_components.insert(reading_components.begin() + insertion_index, type);
	}
	template<typename Write>
	void add_write_dependency()
	{
		std::type_index type = typeid(Write);
		size_t insertion_index = 0;
		for (; insertion_index < writing_components.size(); ++insertion_index)
		{
			auto& write_comp = writing_components[insertion_index];
			if (type < write_comp)
			{
				break;
			}
		}
		writing_components.insert(writing_components.begin() + insertion_index, type);
	}

	template<typename Read>
	void remove_read_dependency()
	{
		std::type_index type = typeid(Read);
		for (size_t i = 0; i < reading_components.size(); i++)
		{
			auto& read_comp = reading_components[i];
			if (type == read_comp)
			{
				reading_components.erase(reading_components.begin() + i);
				break;
			}
		}
	}
	template<typename Write>
	void remove_write_dependency()
	{
		std::type_index type = typeid(Write);
		for (size_t i = 0; i < writing_components.size(); i++)
		{
			auto& write_comp = writing_components[i];
			if (type == write_comp)
			{
				writing_components.erase(writing_components.begin() + i);
				break;
			}
		}
	}
};

struct SystemsStorageObjectBase
{
	std::condition_variable cv;
	std::mutex mutex;
	int waiting_readers = 0;
	int readers = 0;
	int waiting_writers = 0;
	bool writing = false;
};

template<typename T>
struct SystemsStorageObject : SystemsStorageObjectBase
{
	SystemsStorageObject(T a_data) : data(std::move(a_data)) {}
	T data;
	void read(const std::function <void(const T&)>& func)
	{
		std::unique_lock<std::mutex> read_lock(mutex);
		waiting_readers++;
		cv.wait(read_lock, [this]() { return !writing && waiting_writers == 0; }); // maybe remove this
		waiting_readers--;
		readers++;
		read_lock.unlock();
		func(data);
		read_lock.lock();
		readers--;
		cv.notify_all();
	}
	void write(const std::function <void(T&)>& func)
	{
		std::unique_lock write_lock(mutex);
		waiting_writers++;
		cv.wait(write_lock, [this]() { return !writing && readers == 0; });
		waiting_writers--;
		writing = true;
		write_lock.unlock();
		func(data);
		write_lock.lock();
		writing = false;
	}

	void copy(T* data_copy)
	{ 
		std::unique_lock<std::mutex> read_lock(mutex);
		waiting_readers++;
		cv.wait(read_lock, [this]() { return !writing; });
		waiting_readers--;
		readers++;
		read_lock.unlock();
		*data_copy = data;
		read_lock.lock();
		readers--;
		cv.notify_all();
	};
};

struct SystemsContextStorage
{
	using storage_key = std::tuple<std::type_index, std::string>;
	std::map<storage_key, SystemsStorageObjectBase> storage;
	std::vector<storage_key> storage_object_deletions;
	std::mutex deletion_mutex;
	std::mutex storage_mutex;

	template <typename T>
	T* get_object(std::string a_name)
	{
		std::lock_guard storage_guard(storage_mutex);
		if (auto it = storage.find(storage_key(typeid(T), a_name)); it != storage.end())
		{
			return static_cast<SystemsStorageObject<T>*>(&it->second);
		}
		return nullptr;
	}

	//template <typename T>
	//void add_or_replace_object(std::type_index a_type, std::string a_name, T value)
	//{
	//	std::lock_lock storage_lock(storage_mutex);
	//	if (auto it = storage.find(storage_key(typeid(T), a_name)); it != storage.end())
	//	{
	//		SystemsStorageObject<T>* object = static_cast<SystemsStorageObject<T>*>(&it->second);
	//		storage_lock.unlock();
	//		std::unique_lock object_lock(object->mutex);
	//		object->cv.wait(object_lock, [this, &object]() {
	//			return object->readers == 0 && !object->writing;
	//			});
	//		object.data = std::move(value);
	//	}
	//	else
	//	{
	//		storage[storage_key(a_type, a_name)] = SystemsStorageObject<T>(std::move(value));
	//	}
	//	
	//}
	template <typename T>
	void mark_erase(std::string a_name)
	{
		std::lock_guard deletion_guard(deletion_mutex);
		storage_object_deletions.push_back(storage_key(typeid(T), a_name));
	}

	void perform_erase()
	{
		for (auto& deletion : storage_object_deletions)
		{
			auto it = storage.find(deletion);
			if (it != storage.end())
			{
				storage.erase(it);
			}
		}
		storage_object_deletions.clear();
	}
};

struct SystemsContext
{
	SystemsContext(entt::registry& a_registry, std::shared_ptr<WorkerThreadPool> a_thread_pool, std::shared_ptr<WorkerThreadPool> a_general_thread_pool);
	~SystemsContext();

	template<typename... Reads, typename Func>
	void each(WithRead<Reads...> reads, Func&& func)
	{
		while (systems_dependencies.add(reads))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view<Reads...>();
		for (const auto entity : view)
		{
			func(entity, view.template get<Reads>(entity)...);
		}
		systems_dependencies.remove(reads);
	}

	template<typename... Reads, typename... Excludes, typename Func>
	void each(WithRead<Reads...> reads, Without<Excludes...>, Func&& func)
	{
		while (systems_dependencies.add(reads))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view<Reads...>(entt::exclude<Excludes...>);
		for (const auto entity : view)
		{
			func(entity, view.template get<Reads>(entity)...);
		}
		systems_dependencies.remove(reads);
	}

	template<typename... Writes, typename Func>
	void each(WithWrite<Writes...> writes, Func&& func)
	{
		while (systems_dependencies.add(writes))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view< Writes...>();
		for (const auto entity : view)
		{
			func(entity, view.template get<Writes>(entity)...);
		}
		systems_dependencies.remove(writes);
	}

	template<typename... Writes, typename... Excludes, typename Func>
	void each(WithWrite<Writes...> writes, Without<Excludes...>, Func&& func)
	{
		while (systems_dependencies.add(writes))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view<Writes...>(entt::exclude<Excludes...>);
		for (const auto entity : view)
		{
			func(entity, view.template get<Writes>(entity)...);
		}
		systems_dependencies.remove(writes);
	}

	template<typename... Reads, typename... Writes, typename Func>
	void each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Func&& func)
	{
		while (systems_dependencies.add(reads, writes))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view<Reads..., Writes...>();
		for (const auto entity : view)
		{
			func(entity, view.template get<Reads>(entity)...);
		}
		systems_dependencies.remove(reads, writes);
	}

	template<typename... Reads, typename... Writes, typename... Excludes, typename Func>
	void each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Without<Excludes...>, Func&& func)
	{
		while (systems_dependencies.add(reads, writes))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view<Reads..., Writes...>(entt::exclude<Excludes...>);
		for (const auto entity : view)
		{
			func(entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)...);
		}
		systems_dependencies.remove(reads, writes);
	}


	template<typename... Reads, typename Func>
	void enqueue_each(WithRead<Reads...> reads, Func&& func)
	{
		while (systems_dependencies.add(reads))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		entt_thread_pool->enqueue([this, reads, func]() {
			auto view = registry.view<Reads...>();
			for (const auto entity : view)
			{
				func(entity, view.template get<Reads>(entity)...);
			}
			systems_dependencies.remove(reads);
			});
	}

	template<typename... Reads, typename...Excludes, typename Func>
	void enqueue_each(WithRead<Reads...> reads, Without<Excludes...>, Func&& func)
	{
		while (systems_dependencies.add(reads))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		entt_thread_pool->enqueue([this, reads, func]() {
			auto view = registry.view<Reads...>(entt::exclude<Excludes...>);
			for (const auto entity : view)
			{
				func(entity, view.template get<Reads>(entity)...);
			}
			systems_dependencies.remove(reads);
			});
	}

	template<typename... Writes, typename Func>
	void enqueue_each(WithWrite<Writes...>writes , Func&& func)
	{
		while (systems_dependencies.add(writes))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		entt_thread_pool->enqueue([this, writes, func]() {
			auto view = registry.view<Writes...>();
				for (const auto entity : view)
				{
					func(entity, view.template get<Writes>(entity)...);
				}
				systems_dependencies.remove(writes);
			});
	}

	template<typename... Writes, typename...Excludes, typename Func>
	void enqueue_each(WithWrite<Writes...> writes, Without<Excludes...>, Func&& func)
	{
		while (systems_dependencies.add(writes))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		entt_thread_pool->enqueue([this, writes, func]() {
			auto view = registry.view<Writes...>(entt::exclude<Excludes...>);
				for (const auto entity : view)
				{
					func(entity, view.template get<Writes>(entity)...);
				}
				systems_dependencies.remove(writes);
			});
	}

	template<typename... Reads, typename... Writes, typename Func>
	void enqueue_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Func&& func)
	{
		while (systems_dependencies.add(reads, writes))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		entt_thread_pool->enqueue([this, reads, writes, func]() {
			auto view = registry.view<Reads..., Writes...>();
				for (const auto entity : view)
				{
					func(entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)...);
				}
				systems_dependencies.remove(reads, writes);
			});
	}

	template<typename... Reads, typename... Writes, typename...Excludes, typename Func>
	void enqueue_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Without<Excludes...>, Func&& func)
	{
		while (systems_dependencies.add(reads, writes))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		entt_thread_pool->enqueue([this, reads, writes, func]() {
			auto view = registry.view<Reads..., Writes...>(entt::exclude<Excludes...>);
				for (const auto entity : view)
				{
					func(entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)...);
				}
				systems_dependencies.remove(reads, writes);
			});
	}


	template<typename... Reads, typename Func>
	void enqueue_parallel_each(WithRead<Reads...> reads, Func&& func, size_t a_chunk_size = 256)
	{
		while (systems_dependencies.add(reads))
		{
			//PRINTLN("Forced sync");
			each_sync();
		}

		auto view = registry.view<Reads...>();
		const auto* handle = view.handle();
		size_t size = handle->size();

		for (size_t start = 0; start < size; start += a_chunk_size)
		{
			size_t end = std::min(start + a_chunk_size, size);
			systems_dependencies.add(reads, false);

			entt_thread_pool->enqueue([this, start, end, reads, func]()
				{
					auto thread_view = registry.view<Reads...>();
					const auto* thread_handle = thread_view.handle();

					for (size_t i = start; i < end; ++i)
					{
						const entt::entity entity = (*thread_handle)[i];
						if (thread_view.contains(entity))
						{
							func(entity, thread_view.template get<Reads>(entity)...);
						}
					}
					systems_dependencies.remove(reads);
				});
		}
		systems_dependencies.remove(reads);
	}

	template<typename... Reads, typename... Excludes, typename Func>
	void enqueue_parallel_each(WithRead<Reads...> reads, Without<Excludes...>, Func&& func, size_t a_chunk_size = 256)
	{
		while (systems_dependencies.add(reads))
		{
			//PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view<Reads...>(entt::exclude<Excludes...>);
		const auto* handle = view.handle();
		size_t size = handle->size();

		for (size_t start = 0; start < size; start += a_chunk_size)
		{
			size_t end = std::min(start + a_chunk_size, size);
			systems_dependencies.add(reads, false);

			entt_thread_pool->enqueue([this, start, end, reads, func]()
				{
					auto thread_view = registry.view<Reads...>(entt::exclude<Excludes...>);
					const auto* thread_handle = thread_view.handle();

					for (size_t i = start; i < end; ++i)
					{
						const entt::entity entity = (*thread_handle)[i];
						if (thread_view.contains(entity))
						{
							func(entity, thread_view.template get<Reads>(entity)...);
						}
					}
					systems_dependencies.remove(reads);
				});
		}
		systems_dependencies.remove(reads);
	}

	template<typename... Writes, typename Func>
	void enqueue_parallel_each(WithWrite<Writes...> writes, Func&& func, size_t a_chunk_size = 256)
	{
		while (systems_dependencies.add(writes))
		{
			//PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view<Writes...>();
		const auto* handle = view.handle();
		size_t size = handle->size();

		for (size_t start = 0; start < size; start += a_chunk_size)
		{
			size_t end = std::min(start + a_chunk_size, size);
			systems_dependencies.add(writes, false);

			entt_thread_pool->enqueue([this, start, end, writes, func]()
				{
					auto thread_view = registry.view<Writes...>();
					const auto* thread_handle = thread_view.handle();

					for (size_t i = start; i < end; ++i)
					{
						const entt::entity entity = (*thread_handle)[i];
						if (thread_view.contains(entity))
						{
							func(entity, thread_view.template get<Writes>(entity)...);
						}
					}
					systems_dependencies.remove(writes);
				});
		}
		systems_dependencies.remove(writes);
	}

	template<typename... Writes, typename... Excludes, typename Func>
	void enqueue_parallel_each(WithWrite<Writes...> writes, Without<Excludes...>, Func&& func, size_t a_chunk_size = 256)
	{
		while (systems_dependencies.add(writes))
		{
			//PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view<Writes...>(entt::exclude<Excludes...>);
		const auto* handle = view.handle();
		size_t size = handle->size();

		for (size_t start = 0; start < size; start += a_chunk_size)
		{
			size_t end = std::min(start + a_chunk_size, size);
			systems_dependencies.add(writes, false);

			entt_thread_pool->enqueue([this, start, end, writes, func]()
				{
					auto thread_view = registry.view<Writes...>(entt::exclude<Excludes...>);
					const auto* thread_handle = thread_view.handle();

					for (size_t i = start; i < end; ++i)
					{
						const entt::entity entity = (*thread_handle)[i];
						if (thread_view.contains(entity))
						{
							func(entity, thread_view.template get<Writes>(entity)...);
						}
					}
					systems_dependencies.remove(writes);
				});
		}
		systems_dependencies.remove(writes);
	}

	template<typename... Reads, typename... Writes, typename Func>
	void enqueue_parallel_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Func&& func, size_t a_chunk_size = 256)
	{
		while (systems_dependencies.add(reads, writes))
		{
			each_sync();
		}
		auto view = registry.view<Reads..., Writes...>();
		const auto* handle = view.handle();
		size_t size = handle->size();

		for (size_t start = 0; start < size; start += a_chunk_size)
		{
			size_t end = std::min(start + a_chunk_size, size);
			systems_dependencies.add(reads, writes, false);
			entt_thread_pool->enqueue([this, start, end, reads, writes, func]()
				{
					auto thread_view = registry.view<Reads..., Writes...>();
					const auto* thread_handle = thread_view.handle();

					for (size_t i = start; i < end; ++i)
					{
						const entt::entity entity = (*thread_handle)[i];
						if (thread_view.contains(entity))
						{
							func(entity, thread_view.template get<Reads>(entity)..., thread_view.template get<Writes>(entity)...);
						}
					}
					systems_dependencies.remove(reads, writes);
				});
		}
		systems_dependencies.remove(reads, writes);
	}

	template<typename... Reads, typename... Writes, typename... Excludes, typename Func>
	void enqueue_parallel_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Without<Excludes...>, Func&& func, size_t a_chunk_size = 256)
	{
		while (systems_dependencies.add(reads, writes))
		{
			//PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view<Reads..., Writes...>(entt::exclude<Excludes...>);
		const auto* handle = view.handle();
		size_t size = handle->size();

		for (size_t start = 0; start < size; start += a_chunk_size)
		{
			size_t end = std::min(start + a_chunk_size, size);
			systems_dependencies.add(reads, writes, false);
			entt_thread_pool->enqueue([this, start, end, reads, writes, func]()
				{
					auto view = registry.view<Reads..., Writes...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();

					for (size_t i = start; i < end; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)...);
						}
					}
					systems_dependencies.remove(reads, writes);
				});
		}
		systems_dependencies.remove(reads, writes);
	}
	//This AI suggestion is actually dumb, it wants to have the single thread compute everything 
	//before issueing calls, which technically means faster total time the single thread spends in this scope
	//but that that also means that all threads have to wait before working, which they woulndn't need to if
	//they're issued immedietly, as they chew work WHILE the main thread is issueing commands.
#pragma region AIOptimizationSuggestion
	/////////
	// AI suggested optimization.

	//template<typename... Reads, typename Func>
	//void enqueue_parallel_each(WithRead<Reads...> reads, Func&& func, size_t a_chunk_size = 256)
	//{
	//	while (systems_dependencies.add(reads))
	//	{
	//		//PRINTLN("Forced sync");
	//		each_sync();
	//	}

	//	auto view = registry.view<Reads...>();

	//	// Pre-calculate all chunk boundaries
	//	using IterType = decltype(view.begin());
	//	std::vector<std::pair<IterType, IterType>> chunks;

	//	auto it = view.begin();
	//	auto view_end = view.end();

	//	while (it != view_end)
	//	{
	//		auto chunk_begin = it;
	//		size_t chunk_count = 0;

	//		while (it != view_end && chunk_count < a_chunk_size)
	//		{
	//			++it;
	//			++chunk_count;
	//		}

	//		chunks.emplace_back(chunk_begin, it);
	//	}

	//	// Enqueue all chunks rapidly
	//	for (const auto& [chunk_begin, chunk_end] : chunks)
	//	{
	//		systems_dependencies.add(reads, false);
	//		entt_thread_pool->enqueue([this, view, chunk_begin, chunk_end, reads, func]()
	//			{
	//				for (const auto entity_iter = chunk_begin; entity_iter != chunk_end; ++entity_iter)
	//				{
	//					auto entity = *entity_iter;
	//					func(entity, view.template get<Reads>(entity)...);
	//				}
	//				systems_dependencies.remove(reads);
	//			});
	//	}

	//	systems_dependencies.remove(reads);
	//}

	//template<typename... Reads, typename... Excludes, typename Func>
	//void enqueue_parallel_each(WithRead<Reads...> reads, Without<Excludes...>, Func&& func, size_t a_chunk_size = 256)
	//{
	//	while (systems_dependencies.add(reads))
	//	{
	//		//PRINTLN("Forced sync");
	//		each_sync();
	//	}

	//	auto view = registry.view<Reads...>(entt::exclude<Excludes...>);

	//	// Pre-calculate all chunk boundaries
	//	using IterType = decltype(view.begin());
	//	std::vector<std::pair<IterType, IterType>> chunks;

	//	auto it = view.begin();
	//	auto view_end = view.end();

	//	while (it != view_end)
	//	{
	//		auto chunk_begin = it;
	//		size_t chunk_count = 0;

	//		while (it != view_end && chunk_count < a_chunk_size)
	//		{
	//			++it;
	//			++chunk_count;
	//		}

	//		chunks.emplace_back(chunk_begin, it);
	//	}

	//	// Enqueue all chunks rapidly
	//	for (const auto& [chunk_begin, chunk_end] : chunks)
	//	{
	//		systems_dependencies.add(reads, false);
	//		entt_thread_pool->enqueue([this, view, chunk_begin, chunk_end, reads, func]()
	//			{
	//				for (const auto entity_iter = chunk_begin; entity_iter != chunk_end; ++entity_iter)
	//				{
	//					auto entity = *entity_iter;
	//					func(entity, view.template get<Reads>(entity)...);
	//				}
	//				systems_dependencies.remove(reads);
	//			});
	//	}

	//	systems_dependencies.remove(reads);
	//}

	//template<typename... Writes, typename Func>
	//void enqueue_parallel_each(WithWrite<Writes...> writes, Func&& func, size_t a_chunk_size = 256)
	//{
	//	while (systems_dependencies.add(writes))
	//	{
	//		//PRINTLN("Forced sync");
	//		each_sync();
	//	}

	//	auto view = registry.view<Writes...>();

	//	// Pre-calculate all chunk boundaries
	//	using IterType = decltype(view.begin());
	//	std::vector<std::pair<IterType, IterType>> chunks;

	//	auto it = view.begin();
	//	auto view_end = view.end();

	//	while (it != view_end)
	//	{
	//		auto chunk_begin = it;
	//		size_t chunk_count = 0;

	//		while (it != view_end && chunk_count < a_chunk_size)
	//		{
	//			++it;
	//			++chunk_count;
	//		}

	//		chunks.emplace_back(chunk_begin, it);
	//	}

	//	// Enqueue all chunks rapidly
	//	for (const auto& [chunk_begin, chunk_end] : chunks)
	//	{
	//		systems_dependencies.add(writes, false);
	//		entt_thread_pool->enqueue([this, view, chunk_begin, chunk_end, writes, func]()
	//			{
	//				for (const auto entity_iter = chunk_begin; entity_iter != chunk_end; ++entity_iter)
	//				{
	//					auto entity = *entity_iter;
	//					func(entity, view.template get<Writes>(entity)...);
	//				}
	//				systems_dependencies.remove(writes);
	//			});
	//	}

	//	systems_dependencies.remove(writes);
	//}

	//template<typename... Writes, typename... Excludes, typename Func>
	//void enqueue_parallel_each(WithWrite<Writes...> writes, Without<Excludes...>, Func&& func, size_t a_chunk_size = 256)
	//{
	//	while (systems_dependencies.add(writes))
	//	{
	//		//PRINTLN("Forced sync");
	//		each_sync();
	//	}

	//	auto view = registry.view<Writes...>(entt::exclude<Excludes...>);

	//	// Pre-calculate all chunk boundaries
	//	using IterType = decltype(view.begin());
	//	std::vector<std::pair<IterType, IterType>> chunks;

	//	auto it = view.begin();
	//	auto view_end = view.end();

	//	while (it != view_end)
	//	{
	//		auto chunk_begin = it;
	//		size_t chunk_count = 0;

	//		while (it != view_end && chunk_count < a_chunk_size)
	//		{
	//			++it;
	//			++chunk_count;
	//		}

	//		chunks.emplace_back(chunk_begin, it);
	//	}

	//	// Enqueue all chunks rapidly
	//	for (const auto& [chunk_begin, chunk_end] : chunks)
	//	{
	//		systems_dependencies.add(writes, false);
	//		entt_thread_pool->enqueue([this, view, chunk_begin, chunk_end, writes, func]()
	//			{
	//				for (const auto entity_iter = chunk_begin; entity_iter != chunk_end; ++entity_iter)
	//				{
	//					auto entity = *entity_iter;
	//					func(entity, view.template get<Writes>(entity)...);
	//				}
	//				systems_dependencies.remove(writes);
	//			});
	//	}

	//	systems_dependencies.remove(writes);
	//}

	//template<typename... Reads, typename... Writes, typename Func>
	//void enqueue_parallel_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Func&& func, size_t a_chunk_size = 256)
	//{
	//	while (systems_dependencies.add(reads, writes))
	//	{
	//		//PRINTLN("Forced sync");
	//		each_sync();
	//	}

	//	auto view = registry.view<Reads..., Writes...>();

	//	// Pre-calculate all chunk boundaries
	//	using IterType = decltype(view.begin());
	//	std::vector<std::pair<IterType, IterType>> chunks;

	//	auto it = view.begin();
	//	auto view_end = view.end();

	//	while (it != view_end)
	//	{
	//		auto chunk_begin = it;
	//		size_t chunk_count = 0;

	//		while (it != view_end && chunk_count < a_chunk_size)
	//		{
	//			++it;
	//			++chunk_count;
	//		}

	//		chunks.emplace_back(chunk_begin, it);
	//	}

	//	// Enqueue all chunks rapidly
	//	for (const auto& [chunk_begin, chunk_end] : chunks)
	//	{
	//		systems_dependencies.add(reads, writes, false);
	//		entt_thread_pool->enqueue([this, view, chunk_begin, chunk_end, reads, writes, func]()
	//			{
	//				for (const auto entity_iter = chunk_begin; entity_iter != chunk_end; ++entity_iter)
	//				{
	//					auto entity = *entity_iter;
	//					func(entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)...);
	//				}
	//				systems_dependencies.remove(reads, writes);
	//			});
	//	}

	//	systems_dependencies.remove(reads, writes);
	//}

	//template<typename... Reads, typename... Writes, typename... Excludes, typename Func>
	//void enqueue_parallel_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Without<Excludes...>, Func&& func, size_t a_chunk_size = 256)
	//{
	//	while (systems_dependencies.add(reads, writes))
	//	{
	//		//PRINTLN("Forced sync");
	//		each_sync();
	//	}

	//	auto view = registry.view<Reads..., Writes...>(entt::exclude<Excludes...>);

	//	// Pre-calculate all chunk boundaries
	//	using IterType = decltype(view.begin());
	//	std::vector<std::pair<IterType, IterType>> chunks;

	//	auto it = view.begin();
	//	auto view_end = view.end();

	//	while (it != view_end)
	//	{
	//		auto chunk_begin = it;
	//		size_t chunk_count = 0;

	//		while (it != view_end && chunk_count < a_chunk_size)
	//		{
	//			++it;
	//			++chunk_count;
	//		}

	//		chunks.emplace_back(chunk_begin, it);
	//	}

	//	// Enqueue all chunks rapidly
	//	for (const auto& [chunk_begin, chunk_end] : chunks)
	//	{
	//		systems_dependencies.add(reads, writes, false);
	//		entt_thread_pool->enqueue([this, view, chunk_begin, chunk_end, reads, writes, func]()
	//			{
	//				for (const auto entity_iter = chunk_begin; entity_iter != chunk_end; ++entity_iter)
	//				{
	//					auto entity = *entity_iter;
	//					func(entity, view.template get<Reads>(entity)..., view.get<Writes>(entity)...);
	//				}
	//				systems_dependencies.remove(reads, writes);
	//			});
	//	}

	//	systems_dependencies.remove(reads, writes);
	//}
#pragma endregion

	template <typename Func>
	void enqueue(Func&& func) // this needs it's own entt_thread_pool.
	{
		general_thread_pool->enqueue(std::forward<Func>(func));
	}

	template <typename Component>
	Component* try_get(entt::entity entity)
	{
		return registry.try_get<Component>(entity);
	}

	void each_sync()
	{
		entt_thread_pool->wait();
	}

	void gen_sync()
	{
		general_thread_pool->wait();
	}

	size_t num_threads()
	{
		return entt_thread_pool->get_num_threads();
	}

	void set_delta_time(double a_dt) { delta_time = a_dt; }
	double get_delta_time() const { return delta_time; }

	SystemsContextDependencies& get_system_dependencies() { return systems_dependencies; }
	SystemsContextStorage& get_system_storage() { return systems_storage; }

	uint32_t order = 5000;
private:
	double delta_time = 0;
	std::shared_ptr<Scene> scene;
	std::shared_ptr<WorkerThreadPool> entt_thread_pool;
	std::shared_ptr<WorkerThreadPool> general_thread_pool;
	entt::registry& registry;
	SystemsContextDependencies systems_dependencies;
	SystemsContextStorage systems_storage;
};