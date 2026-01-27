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
#include <engine/resources/data_structs.h>
#include <type_traits>

template<typename...T> struct WithWrite {};
template<typename...T> struct WithRead {};
template<typename...T> struct Without {};

struct SystemsContextDependencies
{
	std::vector<std::type_index> reading_components;
	std::vector<std::type_index> writing_components;
	std::mutex components_mutex;
	//std::atomic<bool> block{false};

	template<typename... Reads, typename... Writes>
	bool add(WithRead<Reads...>, WithWrite<Writes...>, bool check_dependencies = true)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			if (check_dependencies)
				if (dependency_collision({ typeid(Reads)... }, { typeid(Writes)... }))
					return false;
			(add_write_dependency<Writes>(),...);
			(add_read_dependency<Reads>(),...);
		}
		return true;
	}

	template<typename... Reads>
	bool add(WithRead<Reads...>, bool check_dependencies = true)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			if (check_dependencies)
				if (dependency_collision({ typeid(Reads)... }, { }))
					return false;
			(add_read_dependency<Reads>(),...);
		}
		return true;
	}

	template<typename... Writes>
	bool add(WithWrite<Writes...>, bool check_dependencies = true)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			if (check_dependencies)
				if (dependency_collision({ }, { typeid(Writes)... }))
					return false;
			(add_write_dependency<Writes>(),...);
		}
		return true;
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
		//if (block.load(std::memory_order_acquire)) return true;
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

struct SystemsContextStorage
{
	using storage_key = std::tuple<std::type_index, std::string>;
	std::map<storage_key, std::unique_ptr<SystemsStorageObjectBase>> storage;
	std::vector<storage_key> storage_object_deletions;
	std::mutex deletion_mutex;
	std::mutex storage_mutex;

	template <typename T>
	SystemsStorageObject<T>* get_object(std::string a_name)
	{
		std::lock_guard storage_guard(storage_mutex);
		auto it = storage.find(storage_key(typeid(T), a_name));
		if (it != storage.end())
		{
			auto& ptr = it->second;
			return static_cast<SystemsStorageObject<T>*>(it->second.get());
		}
		return nullptr;
	}

	template <typename T>
	void add_or_replace_object(std::string a_name, T&& value)
	{
		std::unique_lock storage_lock(storage_mutex);
		auto it = storage.find(storage_key(typeid(std::decay_t<T>), a_name));
		if (it != storage.end())
		{
			SystemsStorageObject<std::decay_t<T>>* object = static_cast<SystemsStorageObject<std::decay_t<T>>*>(it->second.get());
			storage_lock.unlock();
			std::unique_lock object_lock(object->mutex);
			object->cv.wait(object_lock, [&object]() {
				return object->readers == 0 && !object->writing;
				});
			object->data = value;
		}
		else
		{
			storage[storage_key(typeid(std::decay_t<T>), a_name)] = std::make_unique<SystemsStorageObject<std::decay_t<T>>>(value);
		}
		
	}


	template <typename T>
	SystemsStorageObject<T>* new_or_get_object(std::type_index a_type, std::string a_name, T&& value)
	{
		std::unique_lock storage_lock(storage_mutex);
		if (auto it = storage.find(storage_key(typeid(std::decay_t<T>), a_name)); it != storage.end())
		{
			return static_cast<SystemsStorageObject<std::decay_t<T>>*>(it->second.get());
		}
		else
		{
			storage[storage_key(a_type, a_name)] = std::make_unique<SystemsStorageObject<std::decay_t<T>>>(value);
			return nullptr;
		}

	}

	template <typename T>
	void mark_erase(std::string a_name)
	{
		std::lock_guard deletion_guard(deletion_mutex);
		storage_object_deletions.push_back(storage_key(typeid(T), a_name));
	}

	void perform_erase()
	{
		std::lock_guard storage_guard(storage_mutex);
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

	void clear_all()
	{
		std::lock_guard storage_guard(storage_mutex);
		storage.clear();
	}
};

struct SystemsContext
{
	SystemsContext(entt::registry& a_registry, std::shared_ptr<WorkerThreadPool> a_thread_pool, std::shared_ptr<WorkerThreadPool> a_general_thread_pool);
	~SystemsContext();

	template<typename... Reads, typename Func>
	bool each(WithRead<Reads...> reads, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads))
		{
			synced = true;
			PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Reads...>();
		for (const auto entity : view)
		{
			func(entity, view.template get<Reads>(entity)...);
		}
		systems_dependencies.remove(reads);
		return synced;
	}

	template<typename... Reads, typename... Excludes, typename Func>
	bool each(WithRead<Reads...> reads, Without<Excludes...>, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads))
		{
			synced = true;
			PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Reads...>(entt::exclude<Excludes...>);
		for (const auto entity : view)
		{
			func(entity, view.template get<Reads>(entity)...);
		}
		systems_dependencies.remove(reads);
		return synced;
	}

	template<typename... Writes, typename Func>
	bool each(WithWrite<Writes...> writes, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(writes))
		{
			synced = true;
			PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view< Writes...>();
		for (const auto entity : view)
		{
			func(entity, view.template get<Writes>(entity)...);
		}
		systems_dependencies.remove(writes);
		return synced;
	}

	template<typename... Writes, typename... Excludes, typename Func>
	bool each(WithWrite<Writes...> writes, Without<Excludes...>, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(writes))
		{
			synced = true;
			PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Writes...>(entt::exclude<Excludes...>);
		for (const auto entity : view)
		{
			func(entity, view.template get<Writes>(entity)...);
		}
		systems_dependencies.remove(writes);
		return synced;
	}

	template<typename... Reads, typename... Writes, typename Func>
	bool each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, writes))
		{
			synced = true;
			PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Reads..., Writes...>();
		for (const auto entity : view)
		{
			func(entity, view.template get<Reads>(entity)...);
		}
		systems_dependencies.remove(reads, writes);
		return synced;
	}

	template<typename... Reads, typename... Writes, typename... Excludes, typename Func>
	bool each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Without<Excludes...>, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, writes))
		{
			synced = true;
			PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Reads..., Writes...>(entt::exclude<Excludes...>);
		for (const auto entity : view)
		{
			func(entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)...);
		}
		systems_dependencies.remove(reads, writes);
		return synced;
	}


	template<typename... Reads, typename Func>
	bool enqueue_each(WithRead<Reads...> reads, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads))
		{
			synced = true;
			PRINTLN("Forced sync");
			entt_sync();
		}
		entt_thread_pool->enqueue([this, reads, func]() {
			auto view = registry.view<Reads...>();
			for (const auto entity : view)
			{
				func(entity, view.template get<Reads>(entity)...);
			}
			systems_dependencies.remove(reads);
			});
		return synced;
	}

	template<typename... Reads, typename...Excludes, typename Func>
	bool enqueue_each(WithRead<Reads...> reads, Without<Excludes...>, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads))
		{
			synced = true;
			PRINTLN("Forced sync");
			entt_sync();
		}
		entt_thread_pool->enqueue([this, reads, func]() {
			auto view = registry.view<Reads...>(entt::exclude<Excludes...>);
			for (const auto entity : view)
			{
				func(entity, view.template get<Reads>(entity)...);
			}
			systems_dependencies.remove(reads);
			});
		return synced;
	}

	template<typename... Writes, typename Func>
	bool enqueue_each(WithWrite<Writes...>writes , Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(writes))
		{
			synced = true;
			PRINTLN("Forced sync");
			entt_sync();
		}
		entt_thread_pool->enqueue([this, writes, func]() {
			auto view = registry.view<Writes...>();
				for (const auto entity : view)
				{
					func(entity, view.template get<Writes>(entity)...);
				}
				systems_dependencies.remove(writes);
			});
		return synced;
	}

	template<typename... Writes, typename...Excludes, typename Func>
	bool enqueue_each(WithWrite<Writes...> writes, Without<Excludes...>, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(writes))
		{
			synced = true;
			PRINTLN("Forced sync");
			entt_sync();
		}
		entt_thread_pool->enqueue([this, writes, func]() {
			auto view = registry.view<Writes...>(entt::exclude<Excludes...>);
				for (const auto entity : view)
				{
					func(entity, view.template get<Writes>(entity)...);
				}
				systems_dependencies.remove(writes);
			});
		return synced;
	}

	template<typename... Reads, typename... Writes, typename Func>
	bool enqueue_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, writes))
		{
			synced = true;
			PRINTLN("Forced sync");
			entt_sync();
		}
		entt_thread_pool->enqueue([this, reads, writes, func]() {
			auto view = registry.view<Reads..., Writes...>();
				for (const auto entity : view)
				{
					func(entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)...);
				}
				systems_dependencies.remove(reads, writes);
			});
		return synced;
	}

	template<typename... Reads, typename... Writes, typename...Excludes, typename Func>
	bool enqueue_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Without<Excludes...>, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, writes))
		{
			synced = true;
			PRINTLN("Forced sync");
			entt_sync();
		}
		entt_thread_pool->enqueue([this, reads, writes, func]() {
			auto view = registry.view<Reads..., Writes...>(entt::exclude<Excludes...>);
				for (const auto entity : view)
				{
					func(entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)...);
				}
				systems_dependencies.remove(reads, writes);
			});
		return synced;
	}


	template<typename... Reads, typename Func>
	bool enqueue_parallel_each(WithRead<Reads...> reads, Func&& func, size_t a_chunk_size = 256)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads))
		{
			synced = true;
			//PRINTLN("Forced sync");
			entt_sync();
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
					auto view = registry.view<Reads...>();
					const auto* handle = view.handle();

					for (size_t i = start; i < end; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(entity, view.template get<Reads>(entity)...);
						}
					}
					systems_dependencies.remove(reads);
				});
		}
		systems_dependencies.remove(reads);
		return synced;
	}

	template<typename... Reads, typename... Excludes, typename Func>
	bool enqueue_parallel_each(WithRead<Reads...> reads, Without<Excludes...>, Func&& func, size_t a_chunk_size = 256)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads))
		{
			synced = true;
			//PRINTLN("Forced sync");
			entt_sync();
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
					auto view = registry.view<Reads...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();

					for (size_t i = start; i < end; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(entity, view.template get<Reads>(entity)...);
						}
					}
					systems_dependencies.remove(reads);
				});
		}
		systems_dependencies.remove(reads);
		return synced;
	}

	template<typename... Writes, typename Func>
	bool enqueue_parallel_each(WithWrite<Writes...> writes, Func&& func, size_t a_chunk_size = 256)
	{
		bool synced = false;
		while (!systems_dependencies.add(writes))
		{
			synced = true;
			//PRINTLN("Forced sync");
			entt_sync();
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
					auto view = registry.view<Writes...>();
					const auto* handle = view.handle();

					for (size_t i = start; i < end; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(entity, view.template get<Writes>(entity)...);
						}
					}
					systems_dependencies.remove(writes);
				});
		}
		systems_dependencies.remove(writes);
		return synced;
	}

	template<typename... Writes, typename... Excludes, typename Func>
	bool enqueue_parallel_each(WithWrite<Writes...> writes, Without<Excludes...>, Func&& func, size_t a_chunk_size = 256)
	{
		bool synced = false;
		while (!systems_dependencies.add(writes))
		{
			synced = true;
			//PRINTLN("Forced sync");
			entt_sync();
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
					auto view = registry.view<Writes...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();

					for (size_t i = start; i < end; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(entity, view.template get<Writes>(entity)...);
						}
					}
					systems_dependencies.remove(writes);
				});
		}
		systems_dependencies.remove(writes);
		return synced;
	}

	template<typename... Reads, typename... Writes, typename Func>
	bool enqueue_parallel_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Func&& func, size_t a_chunk_size = 256)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, writes))
		{
			synced = true;
			entt_sync();
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
					auto view = registry.view<Reads..., Writes...>();
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
		return synced;
	}

	template<typename... Reads, typename... Writes, typename... Excludes, typename Func>
	bool enqueue_parallel_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Without<Excludes...>, Func&& func, size_t a_chunk_size = 256)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, writes))
		{
			synced = true;
			//PRINTLN("Forced sync");
			entt_sync();
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
		return synced;
	}


	template<typename... Reads, typename Func, typename T>
	bool enqueue_parallel_data_each(WithRead<Reads...> reads, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads))
		{
			synced = true;
			//PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Reads...>();
		const auto* handle = view.handle();
		size_t size = handle->size();
		size_t chunk_size = (size / a_num_threads) + 1;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(reads, false);

			entt_thread_pool->enqueue([this, thread_id, start, end, reads, func, &a_thread_local_data]()
				{
					auto view = registry.view<Reads...>();
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					for (size_t i = start; i < end; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Reads>(entity)...);
						}
					}
					systems_dependencies.remove(reads);
				});
		}
		systems_dependencies.remove(reads);
		return synced;
	}

	template<typename... Reads, typename... Excludes, typename Func, typename T>
	bool enqueue_parallel_data_each(WithRead<Reads...> reads, Without<Excludes...>, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads))
		{
			synced = true;
			//PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Reads...>();
		const auto* handle = view.handle();
		size_t size = handle->size();
		size_t chunk_size = (size / a_num_threads) + 1;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(reads, false);

			entt_thread_pool->enqueue([this, thread_id, start, end, reads, func, &a_thread_local_data]()
				{
					auto view = registry.view<Reads...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					for (size_t i = start; i < end; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Reads>(entity)...);
						}
					}
					systems_dependencies.remove(reads);
				});
		}
		systems_dependencies.remove(reads);
		return synced;
	}

	template<typename... Writes, typename Func, typename T>
	bool enqueue_parallel_data_each(WithWrite<Writes...> writes, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads)
	{
		bool synced = false;
		while (!systems_dependencies.add(writes))
		{
			synced = true;
			//PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Writes...>();
		const auto* handle = view.handle();
		size_t size = handle->size();
		size_t chunk_size = (size / a_num_threads) + 1;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(writes, false);

			entt_thread_pool->enqueue([this, thread_id, start, end, writes, func, &a_thread_local_data]()
				{
					auto view = registry.view<Writes...>();
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					for (size_t i = start; i < end; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Writes>(entity)...);
						}
					}
					systems_dependencies.remove(writes);
				});
		}
		systems_dependencies.remove(writes);
		return synced;
	}

	template<typename... Writes, typename... Excludes, typename Func, typename T>
	bool enqueue_parallel_data_each(WithWrite<Writes...> writes, Without<Excludes...>, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads)
	{
		bool synced = false;
		while (!systems_dependencies.add(writes))
		{
			synced = true;
			//PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Writes...>();
		const auto* handle = view.handle();
		size_t size = handle->size();
		size_t chunk_size = (size / a_num_threads) + 1;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(writes, false);

			entt_thread_pool->enqueue([this, thread_id, start, end, writes, func, &a_thread_local_data]()
				{
					auto view = registry.view<Writes...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					for (size_t i = start; i < end; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Writes>(entity)...);
						}
					}
					systems_dependencies.remove(writes);
				});
		}
		systems_dependencies.remove(writes);
		return synced;
	}

	template<typename... Reads, typename... Writes, typename Func, typename T>
	bool enqueue_parallel_data_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, writes))
		{
			synced = true;
			entt_sync();
		}
		auto view = registry.view<Reads..., Writes...>();
		const auto* handle = view.handle();
		size_t size = handle->size();
		size_t chunk_size = (size / a_num_threads) + 1;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(reads, writes, false);
			entt_thread_pool->enqueue([this, thread_id, start, end, reads, writes, func, &a_thread_local_data]()
				{
					auto view = registry.view<Reads..., Writes...>();
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					for (size_t i = start; i < end; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)...);
						}
					}
					systems_dependencies.remove(reads, writes);
				});
		}
		systems_dependencies.remove(reads, writes);
		return synced;
	}

	template<typename... Reads, typename... Writes, typename... Excludes, typename Func, typename T>
	bool enqueue_parallel_data_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Without<Excludes...>, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, writes))
		{
			synced = true;
			//PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Reads..., Writes...>();
		const auto* handle = view.handle();
		size_t size = handle->size();
		size_t chunk_size = (size / a_num_threads) + 1;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(reads, writes, false);
			entt_thread_pool->enqueue([this, thread_id, start, end, reads, writes, func]()
				{
					auto view = registry.view<Reads..., Writes...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					for (size_t i = start; i < end; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)...);
						}
					}
					systems_dependencies.remove(reads, writes);
				});
		}
		systems_dependencies.remove(reads, writes);
		return synced;
	}

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

	void entt_sync()
	{
		// this has to signal conditions.
		//systems_dependencies.block.store(true, std::memory_order_release)
		entt_thread_pool->wait();
		//systems_dependencies.block.store(false, std::memory_order_release)
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