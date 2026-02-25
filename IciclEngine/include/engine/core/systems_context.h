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
#include <engine/core/entity_command_buffer.h>
#include <engine/core/system_dependencies.h>

struct EngineContext;

struct SystemsContextStorage
{
	using storage_key = std::tuple<std::type_index, std::string, size_t>;
	std::map<storage_key, std::unique_ptr<SystemsStorageObjectBase>> storage;
	std::vector<storage_key> storage_object_deletions;
	std::mutex deletion_mutex;
	std::mutex storage_mutex;

	template <typename T>
	void mark_erase(const std::string& a_name, size_t index = 0)
	{
		std::lock_guard deletion_guard(deletion_mutex);
		storage_object_deletions.push_back(storage_key(typeid(T), a_name, index));
	}

	template <typename T>
	SystemsStorageObject<T>* get_object(const std::string& a_name, size_t index = 0)
	{
		std::lock_guard storage_guard(storage_mutex);
		auto it = storage.find(storage_key(typeid(T), a_name, index));
		if (it != storage.end())
		{
			auto& ptr = it->second;
			return static_cast<SystemsStorageObject<T>*>(ptr.get());
		}
		return nullptr;
	}

	template <typename T>
	std::unique_ptr<T> consume_object(const std::string& a_name, size_t index = 0) // this will leave object unsafe
	{
		std::unique_lock storage_lock(storage_mutex);
		auto it = storage.find(storage_key(typeid(T), a_name, index));
		if (it != storage.end())
		{
			SystemsStorageObject<std::decay_t<T>>* object = static_cast<SystemsStorageObject<std::decay_t<T>>*>(it->second.get());
			//storage_lock.unlock();
			std::unique_lock object_lock(object->mutex);
			object->cv.wait(object_lock, [&object]() {
				return object->readers == 0 && !object->writing;
				});
			std::unique_ptr<T> o = std::make_unique<T>(std::move(object->data));
			object->invalid = true;
			object_lock.unlock();
			object->cv.notify_all();
			mark_erase<T>(a_name, index);
			return std::move(o);
		}
		return nullptr;
	}

	template <typename T>
	void add_or_replace_object(const std::string& a_name, T&& value, size_t index = 0)
	{
		std::unique_lock storage_lock(storage_mutex);
		auto it = storage.find(storage_key(typeid(std::decay_t<T>), a_name, index));
		if (it != storage.end())
		{
			SystemsStorageObject<std::decay_t<T>>* object = static_cast<SystemsStorageObject<std::decay_t<T>>*>(it->second.get());
			storage_lock.unlock();
			std::unique_lock object_lock(object->mutex); // I really should just use write...
			object->cv.wait(object_lock, [&object]() {
				return object->readers == 0 && !object->writing;
				});
			object->data = value;
			object->cv.notify_all();
		}
		else
		{
			storage[storage_key(typeid(std::decay_t<T>), a_name, index)] = std::make_unique<SystemsStorageObject<std::decay_t<T>>>(value);
		}
		
	}

	template <typename T>
	SystemsStorageObject<T>* new_or_get_object(std::type_index a_type, const std::string& a_name, T&& value, size_t index)
	{
		std::unique_lock storage_lock(storage_mutex);
		if (auto it = storage.find(storage_key(typeid(std::decay_t<T>), a_name, index)); it != storage.end())
		{
			return static_cast<SystemsStorageObject<std::decay_t<T>>*>(it->second.get());
		}
		else
		{
			storage[storage_key(a_type, a_name, index)] = std::make_unique<SystemsStorageObject<std::decay_t<T>>>(value);
			return nullptr;
		}

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

	template<typename... Reads, typename... Mods, typename Func>
	bool each(WithRead<Reads...> reads, WithRef<Mods...> mods, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, mods))
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
		systems_dependencies.remove(reads, mods);
		return synced;
	}

	template<typename... Reads, typename... Excludes, typename Func>
	bool each(WithRead<Reads...> reads, WithOut<Excludes...>, Func&& func)
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

	template<typename... Reads, typename... Excludes, typename... Mods, typename Func>
	bool each(WithRead<Reads...> reads, WithOut<Excludes...>, WithRef<Mods...> mods, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, mods))
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
		systems_dependencies.remove(reads, mods);
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

	template<typename... Writes, typename... Mods, typename Func>
	bool each(WithWrite<Writes...> writes, WithRef<Mods...> mods, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(writes, mods))
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
		systems_dependencies.remove(writes, mods);
		return synced;
	}

	template<typename... Writes, typename... Excludes, typename Func>
	bool each(WithWrite<Writes...> writes, WithOut<Excludes...>, Func&& func)
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

	template<typename... Writes, typename... Excludes, typename... Mods, typename Func>
	bool each(WithWrite<Writes...> writes, WithOut<Excludes...>, WithRef<Mods...> mods, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(writes, mods))
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
		systems_dependencies.remove(writes, mods);
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

	template<typename... Reads, typename... Writes, typename...Mods, typename Func>
	bool each(WithRead<Reads...> reads, WithWrite<Writes...> writes, WithRef<Mods...> mods, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, writes, mods))
		{
			synced = true;
			PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Reads..., Writes...>();
		for (const auto entity : view)
		{
			func(entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)...);
		}
		systems_dependencies.remove(reads, writes, mods);
		return synced;
	}

	template<typename... Reads, typename... Writes, typename... Excludes, typename Func>
	bool each(WithRead<Reads...> reads, WithWrite<Writes...> writes, WithOut<Excludes...>, Func&& func)
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

	template<typename... Reads, typename... Writes, typename... Excludes, typename... Mods, typename Func>
	bool each(WithRead<Reads...> reads, WithWrite<Writes...> writes, WithOut<Excludes...>, WithRef<Mods...> mods, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, writes, mods))
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
		systems_dependencies.remove(reads, writes, mods);
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

	template<typename... Reads, typename... Mods, typename Func>
	bool enqueue_each(WithRead<Reads...> reads, WithRef<Mods...> mods, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, mods))
		{
			synced = true;
			PRINTLN("Forced sync");
			entt_sync();
		}
		entt_thread_pool->enqueue([this, reads, mods, func]() {
			auto view = registry.view<Reads...>();
			for (const auto entity : view)
			{
				func(entity, view.template get<Reads>(entity)...);
			}
			systems_dependencies.remove(reads, mods);
			});
		return synced;
	}

	template<typename... Reads, typename...Excludes, typename Func>
	bool enqueue_each(WithRead<Reads...> reads, WithOut<Excludes...>, Func&& func)
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

	template<typename... Reads, typename...Excludes, typename...Mods, typename Func>
	bool enqueue_each(WithRead<Reads...> reads, WithOut<Excludes...>, WithRef<Mods...> mods, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, mods))
		{
			synced = true;
			PRINTLN("Forced sync");
			entt_sync();
		}
		entt_thread_pool->enqueue([this, reads, mods, func]() {
			auto view = registry.view<Reads...>(entt::exclude<Excludes...>);
			for (const auto entity : view)
			{
				func(entity, view.template get<Reads>(entity)...);
			}
			systems_dependencies.remove(reads, mods);
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

	template<typename... Writes, typename...Mods, typename Func>
	bool enqueue_each(WithWrite<Writes...>writes, WithRef<Mods...> mods, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(writes, mods))
		{
			synced = true;
			PRINTLN("Forced sync");
			entt_sync();
		}
		entt_thread_pool->enqueue([this, writes, mods, func]() {
			auto view = registry.view<Writes...>();
			for (const auto entity : view)
			{
				func(entity, view.template get<Writes>(entity)...);
			}
			systems_dependencies.remove(writes, mods);
			});
		return synced;
	}

	template<typename... Writes, typename...Excludes, typename Func>
	bool enqueue_each(WithWrite<Writes...> writes, WithOut<Excludes...>, Func&& func)
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

	template<typename... Writes, typename...Excludes, typename...Mods, typename Func>
	bool enqueue_each(WithWrite<Writes...> writes, WithOut<Excludes...>, WithRef<Mods...> mods, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(writes, mods))
		{
			synced = true;
			PRINTLN("Forced sync");
			entt_sync();
		}
		entt_thread_pool->enqueue([this, writes, mods, func]() {
			auto view = registry.view<Writes...>(entt::exclude<Excludes...>);
			for (const auto entity : view)
			{
				func(entity, view.template get<Writes>(entity)...);
			}
			systems_dependencies.remove(writes, mods);
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

	template<typename... Reads, typename... Writes, typename...Mods, typename Func>
		bool enqueue_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, WithRef<Mods...> mods, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, writes, mods))
		{
			synced = true;
			PRINTLN("Forced sync");
			entt_sync();
		}
		entt_thread_pool->enqueue([this, reads, writes, mods, func]() {
			auto view = registry.view<Reads..., Writes...>();
			for (const auto entity : view)
			{
				func(entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)...);
			}
			systems_dependencies.remove(reads, writes, mods);
			});
		return synced;
	}

	template<typename... Reads, typename... Writes, typename...Excludes, typename Func>
	bool enqueue_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, WithOut<Excludes...>, Func&& func)
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

	template<typename... Reads, typename... Writes, typename...Excludes, typename...Mods, typename Func>
	bool enqueue_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, WithOut<Excludes...>, WithRef<Mods...> mods, Func&& func)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, writes, mods))
		{
			synced = true;
			PRINTLN("Forced sync");
			entt_sync();
		}
		entt_thread_pool->enqueue([this, reads, writes, mods, func]() {
			auto view = registry.view<Reads..., Writes...>(entt::exclude<Excludes...>);
			for (const auto entity : view)
			{
				func(entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)...);
			}
			systems_dependencies.remove(reads, writes, mods);
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

			entt_thread_pool->enqueue([this, view, start, end, reads, func]()
				{
					//auto view = registry.view<Reads...>();
					const auto* handle = view.handle();
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
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

	template<typename... Reads, typename...Mods, typename Func>
	bool enqueue_parallel_each(WithRead<Reads...> reads, WithRef<Mods...> mods, Func&& func, size_t a_chunk_size = 256)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, mods))
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
			systems_dependencies.add(reads, mods, false);

			entt_thread_pool->enqueue([this, view, start, end, reads, mods, func]()
				{
					//auto view = registry.view<Reads...>();
					const auto* handle = view.handle();
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(entity, view.template get<Reads>(entity)...);
						}
					}
					systems_dependencies.remove(reads, mods);
				});
		}
		systems_dependencies.remove(reads, mods);
		return synced;
	}

	template<typename... Reads, typename... Excludes, typename Func>
	bool enqueue_parallel_each(WithRead<Reads...> reads, WithOut<Excludes...>, Func&& func, size_t a_chunk_size = 256)
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

			entt_thread_pool->enqueue([this, view,  start, end, reads, func]()
				{
					//auto view = registry.view<Reads...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
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

	template<typename... Reads, typename... Excludes, typename...Mods, typename Func>
	bool enqueue_parallel_each(WithRead<Reads...> reads, WithOut<Excludes...>, WithRef<Mods...> mods, Func&& func, size_t a_chunk_size = 256)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, mods))
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
			systems_dependencies.add(reads, mods, false);

			entt_thread_pool->enqueue([this, view, start, end, reads, mods, func]()
				{
					//auto view = registry.view<Reads...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(entity, view.template get<Reads>(entity)...);
						}
					}
					systems_dependencies.remove(reads, mods);
				});
		}
		systems_dependencies.remove(reads, mods);
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

			entt_thread_pool->enqueue([this, view, start, end, writes, func]()
				{
					//auto view = registry.view<Writes...>();
					const auto* handle = view.handle();
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
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

	template<typename... Writes, typename...Mods, typename Func>
	bool enqueue_parallel_each(WithWrite<Writes...> writes, WithRef<Mods...> mods, Func&& func, size_t a_chunk_size = 256)
	{
		bool synced = false;
		while (!systems_dependencies.add(writes, mods))
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
			systems_dependencies.add(writes, mods, false);

			entt_thread_pool->enqueue([this, view, start, end, writes, mods, func]()
				{
					//auto view = registry.view<Writes...>();
					const auto* handle = view.handle();
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(entity, view.template get<Writes>(entity)...);
						}
					}
					systems_dependencies.remove(writes, mods);
				});
		}
		systems_dependencies.remove(writes, mods);
		return synced;
	}

	template<typename... Writes, typename... Excludes, typename Func>
	bool enqueue_parallel_each(WithWrite<Writes...> writes, WithOut<Excludes...>, Func&& func, size_t a_chunk_size = 256)
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

			entt_thread_pool->enqueue([this, view, start, end, writes, func]() // maybe if we copy view?
				{
					//auto view = registry.view<Writes...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();
					////////////////////////////////////////////////////////////////////////////////////////////
					// NOTE THIS FIX, PUT THIS IN EVERY SINGLE ONE!!!
					// Still crashes on view<transform, cameracomp> exclude camera comp ... why did this happen??
					// 1k rigid in debug...
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
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

	template<typename... Writes, typename... Excludes, typename...Mods, typename Func>
	bool enqueue_parallel_each(WithWrite<Writes...> writes, WithOut<Excludes...>, WithRef<Mods...> mods, Func&& func, size_t a_chunk_size = 256)
	{
		bool synced = false;
		while (!systems_dependencies.add(writes, mods))
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
			systems_dependencies.add(writes, mods, false);

			entt_thread_pool->enqueue([this, view, start, end, writes, mods, func]()
				{
					//auto view = registry.view<Writes...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(entity, view.template get<Writes>(entity)...);
						}
					}
					systems_dependencies.remove(writes, mods);
				});
		}
		systems_dependencies.remove(writes, mods);
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
			entt_thread_pool->enqueue([this, view, start, end, reads, writes, func]()
				{
					//auto view = registry.view<Reads..., Writes...>();
					const auto* handle = view.handle();
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
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

	template<typename... Reads, typename... Writes, typename...Mods, typename Func>
	bool enqueue_parallel_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, WithRef<Mods...> mods, Func&& func, size_t a_chunk_size = 256)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, writes, mods))
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
			systems_dependencies.add(reads, writes, mods, false);
			entt_thread_pool->enqueue([this, view, start, end, reads, writes, mods, func]()
				{
					//auto view = registry.view<Reads..., Writes...>();
					const auto* handle = view.handle();
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)...);
						}
					}
					systems_dependencies.remove(reads, writes, mods);
				});
		}
		systems_dependencies.remove(reads, writes, mods);
		return synced;
	}

	template<typename... Reads, typename... Writes, typename... Excludes, typename Func>
	bool enqueue_parallel_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, WithOut<Excludes...>, Func&& func, size_t a_chunk_size = 256)
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
			entt_thread_pool->enqueue([this, view, start, end, reads, writes, func]()
				{
					//auto view = registry.view<Reads..., Writes...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
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

	template<typename... Reads, typename... Writes, typename... Excludes, typename...Mods, typename Func>
	bool enqueue_parallel_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, WithOut<Excludes...>, WithRef<Mods...> mods, Func&& func, size_t a_chunk_size = 256)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, writes, mods))
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
			systems_dependencies.add(reads, writes, mods, false);
			entt_thread_pool->enqueue([this, view, start, end, reads, writes, mods, func]()
				{
					//auto view = registry.view<Reads..., Writes...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)...);
						}
					}
					systems_dependencies.remove(reads, writes, mods);
				});
		}
		systems_dependencies.remove(reads, writes, mods);
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
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(reads, false);

			entt_thread_pool->enqueue([this, view, thread_id, start, end, reads, func, &a_thread_local_data]()
				{
					//auto view = registry.view<Reads...>();
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
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

	template<typename... Reads, typename...Mods, typename Func, typename T>
	bool enqueue_parallel_data_each(WithRead<Reads...> reads, WithRef<Mods...> mods, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, mods))
		{
			synced = true;
			//PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Reads...>();
		const auto* handle = view.handle();
		size_t size = handle->size();
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(reads, mods, false);

			entt_thread_pool->enqueue([this, view, thread_id, start, end, reads, mods, func, &a_thread_local_data]()
				{
					//auto view = registry.view<Reads...>();
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Reads>(entity)...);
						}
					}
					systems_dependencies.remove(reads, mods);
				});
		}
		systems_dependencies.remove(reads, mods);
		return synced;
	}

	template<typename... Reads, typename... Excludes, typename Func, typename T>
	bool enqueue_parallel_data_each(WithRead<Reads...> reads, WithOut<Excludes...>, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads)
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
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(reads, false);

			entt_thread_pool->enqueue([this, view, thread_id, start, end, reads, func, &a_thread_local_data]()
				{
					//auto view = registry.view<Reads...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
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

	template<typename... Reads, typename... Excludes, typename...Mods, typename Func, typename T>
	bool enqueue_parallel_data_each(WithRead<Reads...> reads, WithOut<Excludes...>, WithRef<Mods...> mods, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, mods))
		{
			synced = true;
			//PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Reads...>(entt::exclude<Excludes...>);
		const auto* handle = view.handle();
		size_t size = handle->size();
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(reads, mods, false);

			entt_thread_pool->enqueue([this, view, thread_id, start, end, reads, mods, func, &a_thread_local_data]()
				{
					//auto view = registry.view<Reads...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Reads>(entity)...);
						}
					}
					systems_dependencies.remove(reads, mods);
				});
		}
		systems_dependencies.remove(reads, mods);
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
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(writes, false);

			entt_thread_pool->enqueue([this, view, thread_id, start, end, writes, func, &a_thread_local_data]()
				{
					//auto view = registry.view<Writes...>();
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
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

	template<typename... Writes, typename...Mods, typename Func, typename T>
	bool enqueue_parallel_data_each(WithWrite<Writes...> writes, WithRef<Mods...> mods, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads)
	{
		bool synced = false;
		while (!systems_dependencies.add(writes, mods))
		{
			synced = true;
			//PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Writes...>();
		const auto* handle = view.handle();
		size_t size = handle->size();
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(writes, mods, false);

			entt_thread_pool->enqueue([this, view, thread_id, start, end, writes, mods, func, &a_thread_local_data]()
				{
					//auto view = registry.view<Writes...>();
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Writes>(entity)...);
						}
					}
					systems_dependencies.remove(writes, mods);
				});
		}
		systems_dependencies.remove(writes, mods);
		return synced;
	}

	template<typename... Writes, typename... Excludes, typename Func, typename T>
	bool enqueue_parallel_data_each(WithWrite<Writes...> writes, WithOut<Excludes...>, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads)
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
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(writes, false);

			entt_thread_pool->enqueue([this, view, thread_id, start, end, writes, func, &a_thread_local_data]()
				{
					//auto view = registry.view<Writes...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
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

	template<typename... Writes, typename... Excludes, typename...Mods, typename Func, typename T>
	bool enqueue_parallel_data_each(WithWrite<Writes...> writes, WithOut<Excludes...>, WithRef<Mods...> mods, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads)
	{
		bool synced = false;
		while (!systems_dependencies.add(writes, mods))
		{
			synced = true;
			//PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Writes...>(entt::exclude<Excludes...>);
		const auto* handle = view.handle();
		size_t size = handle->size();
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(writes, mods, false);

			entt_thread_pool->enqueue([this, view, thread_id, start, end, writes, mods, func, &a_thread_local_data]()
				{
					//auto view = registry.view<Writes...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Writes>(entity)...);
						}
					}
					systems_dependencies.remove(writes, mods);
				});
		}
		systems_dependencies.remove(writes, mods);
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
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(reads, writes, false);
			entt_thread_pool->enqueue([this, view, thread_id, start, end, reads, writes, func, &a_thread_local_data]()
				{
					//auto view = registry.view<Reads..., Writes...>();
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
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

	template<typename... Reads, typename... Writes, typename...Mods, typename Func, typename T>
	bool enqueue_parallel_data_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, WithRef<Mods...> mods, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, writes, mods))
		{
			synced = true;
			entt_sync();
		}
		auto view = registry.view<Reads..., Writes...>();
		const auto* handle = view.handle();
		size_t size = handle->size();
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(reads, writes, mods, false);
			entt_thread_pool->enqueue([this, view, thread_id, start, end, reads, writes, mods, func, &a_thread_local_data]()
				{
					//auto view = registry.view<Reads..., Writes...>();
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)...);
						}
					}
					systems_dependencies.remove(reads, writes, mods);
				});
		}
		systems_dependencies.remove(reads, writes, mods);
		return synced;
	}

	template<typename... Reads, typename... Writes, typename... Excludes, typename Func, typename T>
	bool enqueue_parallel_data_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, WithOut<Excludes...>, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads)
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
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(reads, writes, false);
			entt_thread_pool->enqueue([this, view, thread_id, start, end, reads, writes, func]()
				{
					//auto view = registry.view<Reads..., Writes...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
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

	template<typename... Reads, typename... Writes, typename... Excludes, typename...Mods, typename Func, typename T>
	bool enqueue_parallel_data_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, WithOut<Excludes...>, WithRef<Mods...> mods, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads)
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, writes, mods))
		{
			synced = true;
			//PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Reads..., Writes...>(entt::exclude<Excludes...>);
		const auto* handle = view.handle();
		size_t size = handle->size();
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(reads, writes, mods, false);
			entt_thread_pool->enqueue([this, view, thread_id, start, end, reads, writes, mods, func]()
				{
					//auto view = registry.view<Reads..., Writes...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)...);
						}
					}
					systems_dependencies.remove(reads, writes, mods);
				});
		}
		systems_dependencies.remove(reads, writes, mods);
		return synced;
	}

	template<typename... Reads, typename Func, typename T>
	bool enqueue_parallel_data_each(WithRead<Reads...> reads, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads, bool record_order) // Note, access order is reversed
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
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(reads,false);

			entt_thread_pool->enqueue([this, view, thread_id, start, end, reads, func, &a_thread_local_data]()
				{
					//auto view = registry.view<Reads...>();
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Reads>(entity)..., i);
						}
					}
					systems_dependencies.remove(reads);
				});
		}
		systems_dependencies.remove(reads);
		return synced;
	}

	template<typename... Reads, typename...Mods, typename Func, typename T>
	bool enqueue_parallel_data_each(WithRead<Reads...> reads, WithRef<Mods...> mods, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads, bool record_order) // Note, access order is reversed
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, mods))
		{
			synced = true;
			//PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Reads...>();
		const auto* handle = view.handle();
		size_t size = handle->size();
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(reads, mods, false);

			entt_thread_pool->enqueue([this, view, thread_id, start, end, reads, mods, func, &a_thread_local_data]()
				{
					//auto view = registry.view<Reads...>();
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					//size_t new_size = std::max(local_data.size() + 1, start - end);
					//local_data.reserve(new_size);
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Reads>(entity)..., i);
						}
					}
					systems_dependencies.remove(reads, mods);
				});
		}
		systems_dependencies.remove(reads, mods);
		return synced;
	}

	template<typename... Reads, typename... Excludes, typename Func, typename T>
	bool enqueue_parallel_data_each(WithRead<Reads...> reads, WithOut<Excludes...>, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads, bool record_order) // Note, access order is reversed
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
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(reads, false);

			entt_thread_pool->enqueue([this, view, thread_id, start, end, reads, func, &a_thread_local_data]()
				{
					//auto view = registry.view<Reads...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Reads>(entity)..., i);
						}
					}
					systems_dependencies.remove(reads);
				});
		}
		systems_dependencies.remove(reads);
		return synced;
	}

	template<typename... Reads, typename... Excludes, typename...Mods, typename Func, typename T>
	bool enqueue_parallel_data_each(WithRead<Reads...> reads, WithOut<Excludes...>, WithRef<Mods...> mods, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads, bool record_order) // Note, access order is reversed
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, mods))
		{
			synced = true;
			//PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Reads...>(entt::exclude<Excludes...>);
		const auto* handle = view.handle();
		size_t size = handle->size();
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(reads, mods, false);

			entt_thread_pool->enqueue([this, view, thread_id, start, end, reads, mods, func, &a_thread_local_data]()
				{
					//auto view = registry.view<Reads...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();
					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Reads>(entity)..., i);
						}
					}
					systems_dependencies.remove(reads, mods);
				});
		}
		systems_dependencies.remove(reads, mods);
		return synced;
	}

	
	template<typename... Writes, typename Func, typename T>
	bool enqueue_parallel_data_each(WithWrite<Writes...> writes, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads, bool record_order) // Note, access order is reversed
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
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(writes, false);

			entt_thread_pool->enqueue([this, view, thread_id, start, end, writes, func, &a_thread_local_data]()
				{
					//auto view = registry.view<Writes...>();
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Writes>(entity)..., i);
						}
					}
					systems_dependencies.remove(writes);
				});
		}
		systems_dependencies.remove(writes);
		return synced;
	}

	template<typename... Writes, typename...Mods, typename Func, typename T>
	bool enqueue_parallel_data_each(WithWrite<Writes...> writes, WithRef<Mods...> mods, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads, bool record_order) // Note, access order is reversed
	{
		bool synced = false;
		while (!systems_dependencies.add(writes, mods))
		{
			synced = true;
			//PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Writes...>();
		const auto* handle = view.handle();
		size_t size = handle->size();
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(writes, mods, false);

			entt_thread_pool->enqueue([this, view, thread_id, start, end, writes, mods, func, &a_thread_local_data]()
				{
					//auto view = registry.view<Writes...>();
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Writes>(entity)..., i);
						}
					}
					systems_dependencies.remove(writes, mods);
				});
		}
		systems_dependencies.remove(writes, mods);
		return synced;
	}

	template<typename... Writes, typename... Excludes, typename Func, typename T>
	bool enqueue_parallel_data_each(WithWrite<Writes...> writes, WithOut<Excludes...>, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads, bool record_order) // Note, access order is reversed
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
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(writes, false);

			entt_thread_pool->enqueue([this, view, thread_id, start, end, writes, func, &a_thread_local_data]()
				{
					//auto view = registry.view<Writes...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Writes>(entity)..., i);
						}
					}
					systems_dependencies.remove(writes);
				});
		}
		systems_dependencies.remove(writes);
		return synced;
	}

	template<typename... Writes, typename... Excludes, typename...Mods, typename Func, typename T>
	bool enqueue_parallel_data_each(WithWrite<Writes...> writes, WithOut<Excludes...>, WithRef<Mods...> mods, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads, bool record_order) // Note, access order is reversed
	{
		bool synced = false;
		while (!systems_dependencies.add(writes, mods))
		{
			synced = true;
			//PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Writes...>(entt::exclude<Excludes...>);
		const auto* handle = view.handle();
		size_t size = handle->size();
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(writes, mods, false);

			entt_thread_pool->enqueue([this, view, thread_id, start, end, writes, mods, func, &a_thread_local_data]()
				{
					//auto view = registry.view<Writes...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Writes>(entity)..., i);
						}
					}
					systems_dependencies.remove(writes, mods);
				});
		}
		systems_dependencies.remove(writes, mods);
		return synced;
	}

	template<typename... Reads, typename... Writes, typename Func, typename T>
	bool enqueue_parallel_data_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads, bool record_order) // Note, access order is reversed
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
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(reads, writes, false);
			entt_thread_pool->enqueue([this, view, thread_id, start, end, reads, writes, func, &a_thread_local_data]()
				{
					//auto view = registry.view<Reads..., Writes...>();
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)..., i);
						}
					}
					systems_dependencies.remove(reads, writes);
				});
		}
		systems_dependencies.remove(reads, writes);
		return synced;
	}

	template<typename... Reads, typename... Writes, typename...Mods, typename Func, typename T>
	bool enqueue_parallel_data_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, WithRef<Mods...> mods, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads, bool record_order) // Note, access order is reversed
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, writes, mods))
		{
			synced = true;
			entt_sync();
		}
		auto view = registry.view<Reads..., Writes...>();
		const auto* handle = view.handle();
		size_t size = handle->size();
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(reads, writes, mods, false);
			entt_thread_pool->enqueue([this, view, thread_id, start, end, reads, writes, mods, func, &a_thread_local_data]()
				{
					//auto view = registry.view<Reads..., Writes...>();
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)..., i);
						}
					}
					systems_dependencies.remove(reads, writes, mods);
				});
		}
		systems_dependencies.remove(reads, writes, mods);
		return synced;
	}

	template<typename... Reads, typename... Writes, typename... Excludes, typename Func, typename T>
	bool enqueue_parallel_data_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, WithOut<Excludes...>, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads, bool record_order) // Note, access order is reversed
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
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(reads, writes, false);
			entt_thread_pool->enqueue([this, view, thread_id, start, end, reads, writes, func]()
				{
					//auto view = registry.view<Reads..., Writes...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)..., i);
						}
					}
					systems_dependencies.remove(reads, writes);
				});
		}
		systems_dependencies.remove(reads, writes);
		return synced;
	}

	template<typename... Reads, typename... Writes, typename... Excludes, typename...Mods, typename Func, typename T>
	bool enqueue_parallel_data_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, WithOut<Excludes...>, WithRef<Mods...> mods, Func&& func, std::vector<T>& a_thread_local_data, size_t a_num_threads, bool record_order) // Note, access order is reversed
	{
		bool synced = false;
		while (!systems_dependencies.add(reads, writes, mods))
		{
			synced = true;
			//PRINTLN("Forced sync");
			entt_sync();
		}
		auto view = registry.view<Reads..., Writes...>(entt::exclude<Excludes...>);
		const auto* handle = view.handle();
		size_t size = handle->size();
		size_t chunk_size = (size + a_num_threads - 1) / a_num_threads;

		for (size_t thread_id = 0; thread_id < a_num_threads; ++thread_id)
		{
			size_t start = thread_id * chunk_size;
			size_t end = std::min(start + chunk_size, size);
			if (start >= size) break;
			systems_dependencies.add(reads, writes, mods, false);
			entt_thread_pool->enqueue([this, view, thread_id, start, end, &a_thread_local_data, reads, writes, mods, func]()
				{
					//auto view = registry.view<Reads..., Writes...>(entt::exclude<Excludes...>);
					const auto* handle = view.handle();

					auto& local_data = a_thread_local_data[thread_id];
					if (!handle) return;
					size_t size = handle->size();
					for (size_t i = start; i < end && i < size; ++i)
					{
						const entt::entity entity = (*handle)[i];
						if (view.contains(entity))
						{
							func(local_data, entity, view.template get<Reads>(entity)..., view.template get<Writes>(entity)..., i);
						}
					}
					systems_dependencies.remove(reads, writes, mods);
				});
		}
		systems_dependencies.remove(reads, writes, mods);
		return synced;
	}


	template <typename Func>
	void enqueue(Func&& func) 
	{
		general_thread_pool->enqueue(std::forward<Func>(func));
	}

	// this should work now.
	template <typename...Refs, typename Func>
	void enqueue(WithWrite<Refs...> refs, Func&& func) 
	{
		general_thread_pool->enqueue(
			[func = std::forward<Func>(func), refs]() {
				while (!systems_dependencies.add(refs))
				{
					PRINTLN("Forced poll");
					poll();
				}
				func();
				systems_dependencies.remove(refs);
			}
		);
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

	void entt_poll()
	{
		entt_thread_pool->poll();
	}
	void gen_poll()
	{
		general_thread_pool->poll();
	}

	void sync()
	{
		gen_sync();
		entt_sync();
	}

	void poll()
	{
		gen_poll();
		entt_poll();
	}

	size_t num_threads()
	{
		return entt_thread_pool->get_num_threads();
	}

	void create_ecb(const std::string& ecb_name)
	{
		if (!ecbs.contains(ecb_name))
		{
			ecbs[ecb_name] = EntityCommandBuffer();
		}
	}
	void remove_ecb(const std::string& ecb_name)
	{
		ecbs.erase(ecb_name);
	}

	EntityCommandBuffer* get_ecb(const std::string& ecb_name)
	{
		if (ecbs.contains(ecb_name))
		{
			return &ecbs[ecb_name];
		}
		return nullptr;
	}

	void execute_ecb(const std::string& ecb_name)
	{
		gen_sync();
		entt_sync();
		if (ecbs.contains(ecb_name))
		{
			ecbs[ecb_name].execute_queue(registry);
		}
	}

	void set_delta_time(double a_dt) { delta_time = a_dt; }
	double get_delta_time() const { return delta_time; }
	const std::shared_ptr<EngineContext> get_engine_context();


	SystemsContextDependencies& get_system_dependencies() { return systems_dependencies; }
	SystemsContextStorage& get_system_storage() { return systems_storage; }

	EntityCommandBuffer end_frame_ecb;

	uint32_t order = 5000;
private:
	double delta_time = 0;
	
	std::unordered_map<std::string, EntityCommandBuffer> ecbs;
	std::shared_ptr<Scene> scene;
	std::shared_ptr<WorkerThreadPool> entt_thread_pool;
	std::shared_ptr<WorkerThreadPool> general_thread_pool;
	entt::registry& registry;
	SystemsContextDependencies systems_dependencies;
	SystemsContextStorage systems_storage;
	std::shared_ptr<EngineContext> engine_context;
};