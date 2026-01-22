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

using storage_key = std::tuple<std::type_index, std::string>;

template<typename...T> struct WithWrite {};
template<typename...T> struct WithRead {};
template<typename...T> struct Without {};

struct SystemsContextDependencies
{
	std::vector<std::type_index> reading_components;
	std::vector<std::type_index> writing_components;
	std::mutex components_mutex;

	std::vector<storage_key> reading_storages;
	std::vector<storage_key> writing_storages;

	template<typename... Reads, typename... Writes>
	bool add_component_dependencies(WithRead<Reads...>, WithWrite<Writes...>, bool check_dependencies = true)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			if (check_dependencies)
				if (check_component_collision({ typeid(Reads)... }, { typeid(Writes)... }))
					return true;
			(add_component_write_dependency<Writes>(),...);
			(add_component_read_dependency<Reads>(),...);
		}
		return false;
	}

	template<typename... Reads>
	bool add_component_dependencies(WithRead<Reads...>, bool check_dependencies = true)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			if (check_dependencies)
				if (check_component_collision({ typeid(Reads)... }, { }))
					return true;
			(add_component_read_dependency<Reads>(),...);
		}
		return false;
	}

	template<typename... Writes>
	bool add_component_dependencies(WithWrite<Writes...>, bool check_dependencies = true)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			if (check_dependencies)
				if (check_component_collision({ }, { typeid(Writes)... }))
					return true;
			(add_component_write_dependency<Writes>(),...);
		}
		return false;
	}

	template<typename... Reads, typename... Writes>
	void remove_component_dependencies(WithRead<Reads...>, WithWrite<Writes...>)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			(remove_component_write_dependency<Writes>(), ...);
			(remove_component_read_dependency<Reads>(), ...);
		}
	}

	template<typename... Reads>
	void remove_component_dependencies(WithRead<Reads...>)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			(remove_component_read_dependency<Reads>(), ...);
		}
	}

	template<typename... Writes>
	void remove_component_dependencies(WithWrite<Writes...>)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			(remove_component_write_dependency<Writes>(), ...);
		}
	}
protected:
	bool check_component_collision(std::vector<std::type_index>&& a_reads, std::vector<std::type_index>&& a_writes) // have to actually check if component that is going to be added collides
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
	void add_component_read_dependency()
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
	void add_component_write_dependency()
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
	void remove_component_read_dependency()
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
	void remove_component_write_dependency()
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
	std::map<storage_key, std::any> storage;

	template <typename T>
	T* get_object(std::string a_name)
	{
		if (auto it = storage.find(storage_key(typeid(T), a_name)); it != storage.end())
		{
			return std::any_cast<T>(&it->second);
		}
		return nullptr;
	}

	template <typename T>
	void add_or_replace_object(std::type_index a_type, std::string a_name, T value)
	{
		storage[storage_key(a_type, a_name)] = std::move(value);
	}
};

struct SystemsContext
{
	SystemsContext(entt::registry& a_registry, std::shared_ptr<WorkerThreadPool> a_thread_pool, std::shared_ptr<WorkerThreadPool> a_general_thread_pool);
	~SystemsContext();

	// before going into any of these, it has to register the dependencies

	template<typename... Writes, typename Func>
	void each(WithWrite<Writes...> writes, Func&& func)
	{
		while (systems_dependencies.add_component_dependencies(writes))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view< Writes...>();
		for (auto entity : view)
		{
			func(entity, view.get<Writes>(entity)...);
		}
		systems_dependencies.remove_component_dependencies(writes);
	}

	template<typename... Writes, typename... Excludes, typename Func>
	void each(WithWrite<Writes...> writes, Without<Excludes...>, Func&& func)
	{
		while (systems_dependencies.add_component_dependencies(writes))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view<Writes...>(entt::exclude<Excludes...>);
		for (auto entity : view)
		{
			func(entity, view.get<Writes>(entity)...);
		}
		systems_dependencies.remove_component_dependencies(writes);
	}

	template<typename... Reads, typename Func>
	void each(WithRead<Reads...> reads, Func&& func)
	{
		while (systems_dependencies.add_component_dependencies(reads))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view<Reads...>();
		for (auto entity : view)
		{
			func(entity, view.template get<Reads>(entity)...);
		}
		systems_dependencies.remove_component_dependencies(reads);
	}

	template<typename... Reads, typename... Excludes, typename Func>
	void each(WithRead<Reads...> reads, Without<Excludes...>, Func&& func)
	{
		while (systems_dependencies.add_component_dependencies(reads))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view<Reads...>(entt::exclude<Excludes...>);
		for (auto entity : view)
		{
			func(entity, view.template get<Reads>(entity)...);
		}
		systems_dependencies.remove_component_dependencies(reads);
	}

	template<typename... Reads, typename... Writes, typename Func>
	void each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Func&& func)
	{
		while (systems_dependencies.add_component_dependencies(reads, writes))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view<Reads..., Writes...>();
		for (auto entity : view)
		{
			func(entity, view.template get<Reads>(entity)...);
		}
		systems_dependencies.remove_component_dependencies(reads, writes);
	}

	template<typename... Reads, typename... Writes, typename... Excludes, typename Func>
	void each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Without<Excludes...>, Func&& func)
	{
		while (systems_dependencies.add_component_dependencies(reads, writes))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view<Reads..., Writes...>(entt::exclude<Excludes...>);
		for (auto entity : view)
		{
			func(entity, view.template get<Reads>(entity)..., view.get<Writes>(entity)...);
		}
		systems_dependencies.remove_component_dependencies(reads, writes);
	}

	template<typename... Writes, typename Func>
	void enqueue_each(WithWrite<Writes...>writes , Func&& func)
	{
		while (systems_dependencies.add_component_dependencies(writes))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		entt_thread_pool->enqueue([this, writes, func]() {
			auto view = registry.view<Writes...>();
				for (auto entity : view)
				{
					func(entity, view.template get<Writes>(entity)...);
				}
				systems_dependencies.remove_component_dependencies(writes);
			});
	}

	template<typename... Writes, typename...Excludes, typename Func>
	void enqueue_each(WithWrite<Writes...> writes, Without<Excludes...>, Func&& func)
	{
		while (systems_dependencies.add_component_dependencies(writes))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		entt_thread_pool->enqueue([this, writes, func]() {
			auto view = registry.view<Writes...>(entt::exclude<Excludes...>);
				for (auto entity : view)
				{
					func(entity, view.template get<Writes>(entity)...);
				}
				systems_dependencies.remove_component_dependencies(writes);
			});
	}

	template<typename... Reads, typename Func>
	void enqueue_each(WithRead<Reads...> reads, Func&& func)
	{
		while (systems_dependencies.add_component_dependencies(reads))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		entt_thread_pool->enqueue([this, reads, func]() {
			auto view = registry.view<Reads...>();
				for (auto entity : view)
				{
					func(entity, view.template get<Reads>(entity)...);
				}
				systems_dependencies.remove_component_dependencies(reads);
			});
	}

	template<typename... Reads, typename...Excludes, typename Func>
	void enqueue_each(WithRead<Reads...> reads, Without<Excludes...>, Func&& func)
	{
		while (systems_dependencies.add_component_dependencies(reads))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		entt_thread_pool->enqueue([this, reads, func]() {
			auto view = registry.view<Reads...>(entt::exclude<Excludes...>);
				for (auto entity : view)
				{
					func(entity, view.template get<Reads>(entity)...);
				}
				systems_dependencies.remove_component_dependencies(reads);
			});
	}

	template<typename... Reads, typename... Writes, typename Func>
	void enqueue_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Func&& func)
	{
		while (systems_dependencies.add_component_dependencies(reads, writes))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		entt_thread_pool->enqueue([this, reads, writes, func]() {
			auto view = registry.view<Reads..., Writes...>();
				for (auto entity : view)
				{
					func(entity, view.template get<Reads>(entity)..., view.get<Writes>(entity)...);
				}
				systems_dependencies.remove_component_dependencies(reads, writes);
			});
	}

	template<typename... Reads, typename... Writes, typename...Excludes, typename Func>
	void enqueue_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Without<Excludes...>, Func&& func)
	{
		while (systems_dependencies.add_component_dependencies(reads, writes))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		entt_thread_pool->enqueue([this, reads, writes, func]() {
			auto view = registry.view<Reads..., Writes...>(entt::exclude<Excludes...>);
				for (auto entity : view)
				{
					func(entity, view.template get<Reads>(entity)..., view.get<Writes>(entity)...);
				}
				systems_dependencies.remove_component_dependencies(reads, writes);
			});
	}



	template<typename... Writes, typename Func>
	void enqueue_parallel_each(WithWrite<Writes...> writes, Func&& func, size_t a_chunk_size = 256)
	{
		while (systems_dependencies.add_component_dependencies(writes))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view<Writes...>();
		auto it = view.begin();

		while (it != view.end())
		{
			auto entity_chunk_begin = it;

			size_t chunk_count = 0;
			while (it != view.end() && chunk_count < a_chunk_size)
			{
				++it;
				++chunk_count;
			}
			auto entity_chunk_end = it;
			systems_dependencies.add_component_dependencies(writes, false);
			entt_thread_pool->enqueue([this, view, entity_chunk_begin, entity_chunk_end, func]()
				{
					//auto view = registry.view<Components...>();
					for (auto entity_iter = entity_chunk_begin; entity_iter != entity_chunk_end; ++entity_iter)
					{
						auto entity = *entity_iter;
						func(entity, view.template get<Writes>(entity)...); // needs to match oreder in system
					} // unsure if I need .template ... might need it, might not.
					systems_dependencies.remove_component_dependencies(writes);
				}
			);
		}
		systems_dependencies.remove_component_dependencies(writes);
	}

	template<typename... Writes, typename... Excludes, typename Func>
	void enqueue_parallel_each(WithWrite<Writes...> writes, Without<Excludes...>, Func&& func, size_t a_chunk_size = 256)
	{
		while (systems_dependencies.add_component_dependencies(writes))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view<Writes...>(entt::exclude<Excludes...>);
		auto it = view.begin();

		while (it != view.end())
		{
			auto entity_chunk_begin = it;

			size_t chunk_count = 0;
			while (it != view.end() && chunk_count < a_chunk_size)
			{
				++it;
				++chunk_count;
			}
			auto entity_chunk_end = it;
			systems_dependencies.add_component_dependencies(writes, false);
			entt_thread_pool->enqueue([this, view, entity_chunk_begin, entity_chunk_end, writes, func]()
				{
					//auto view = registry.view<Components...>(entt::exclude<Excludes...>);
					for (auto entity_iter = entity_chunk_begin; entity_iter != entity_chunk_end; ++entity_iter)
					{
						auto entity = *entity_iter;
						func(entity, view.template get<Writes>(entity)...); // needs to match oreder in system
					} // unsure if I need .template ... might need it, might not.
					systems_dependencies.remove_component_dependencies(writes);
				}
			);
		}
		systems_dependencies.remove_component_dependencies(writes);
	}

	template<typename... Reads, typename Func>
	void enqueue_parallel_each(WithRead<Reads...> reads, Func&& func, size_t a_chunk_size = 256)
	{
		while (systems_dependencies.add_component_dependencies(reads))
		{
			PRINTLN("Forced sync");
			each_sync();
		}

		auto view = registry.view<Reads...>();
		auto it = view.begin();

		while (it != view.end())
		{
			auto entity_chunk_begin = it;

			size_t chunk_count = 0;
			while (it != view.end() && chunk_count < a_chunk_size)
			{
				++it;
				++chunk_count;
			}
			auto entity_chunk_end = it;
			systems_dependencies.add_component_dependencies(reads, false);
			entt_thread_pool->enqueue([this, view, entity_chunk_begin, entity_chunk_end, reads, func]()
				{
					//auto view = registry.view<Components...>();
					for (auto entity_iter = entity_chunk_begin; entity_iter != entity_chunk_end; ++entity_iter)
					{
						auto entity = *entity_iter;
						func(entity, view.template get<Reads>(entity)...); // needs to match oreder in system
					} // unsure if I need .template ... might need it, might not.
					systems_dependencies.remove_component_dependencies(reads);
				}
			);
		}
		systems_dependencies.remove_component_dependencies(reads);
	}

	template<typename... Reads, typename... Excludes, typename Func>
	void enqueue_parallel_each(WithRead<Reads...> reads, Without<Excludes...>, Func&& func, size_t a_chunk_size = 256)
	{
		while (systems_dependencies.add_component_dependencies(reads))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view<Reads...>(entt::exclude<Excludes...>);
		auto it = view.begin();

		while (it != view.end())
		{
			auto entity_chunk_begin = it;

			size_t chunk_count = 0;
			while (it != view.end() && chunk_count < a_chunk_size)
			{
				++it;
				++chunk_count;
			}
			auto entity_chunk_end = it;
			systems_dependencies.add_component_dependencies(reads, false);
			entt_thread_pool->enqueue([this, view, entity_chunk_begin, entity_chunk_end, func]()
				{
					//auto view = registry.view<Components...>(entt::exclude<Excludes...>);
					for (auto entity_iter = entity_chunk_begin; entity_iter != entity_chunk_end; ++entity_iter)
					{
						auto entity = *entity_iter;
						func(entity, view.template get<Reads>(entity)...); // needs to match oreder in system
					} // unsure if I need .template ... might need it, might not.
					systems_dependencies.remove_component_dependencies(reads);
				}
			);
		}
	}

	template<typename... Reads, typename... Writes, typename Func>
	void enqueue_parallel_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Func&& func, size_t a_chunk_size = 256)
	{
		while (systems_dependencies.add_component_dependencies(reads ,writes))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view<Reads..., Writes...>();
		auto it = view.begin();

		while (it != view.end())
		{
			auto entity_chunk_begin = it;

			size_t chunk_count = 0;
			while (it != view.end() && chunk_count < a_chunk_size)
			{
				++it;
				++chunk_count;
			}
			auto entity_chunk_end = it;
			systems_dependencies.add_component_dependencies(reads, writes, false);
			entt_thread_pool->enqueue([this, view, entity_chunk_begin, entity_chunk_end, reads, writes, func]()
				{
					//auto view = registry.view<Components...>();
					for (auto entity_iter = entity_chunk_begin; entity_iter != entity_chunk_end; ++entity_iter)
					{
						auto entity = *entity_iter;
						func(entity, view.template get<Reads>(entity)..., view.get<Writes>(entity)...); // needs to match oreder in system
					} // unsure if I need .template ... might need it, might not.
					systems_dependencies.remove_component_dependencies(reads, writes);
				}
			);
		}
		systems_dependencies.remove_component_dependencies(reads, writes);
	}

	template<typename... Reads, typename... Writes, typename... Excludes, typename Func>
	void enqueue_parallel_each(WithRead<Reads...> reads, WithWrite<Writes...> writes, Without<Excludes...>, Func&& func, size_t a_chunk_size = 256)
	{
		while (systems_dependencies.add_component_dependencies(reads, writes))
		{
			PRINTLN("Forced sync");
			each_sync();
		}
		auto view = registry.view<Reads..., Writes...>(entt::exclude<Excludes...>);
		auto it = view.begin();
		
		while (it != view.end())
		{
			auto entity_chunk_begin = it;

			size_t chunk_count = 0;
			while (it != view.end() && chunk_count < a_chunk_size)
			{
				++it;
				++chunk_count;
			}
			auto entity_chunk_end = it;
			systems_dependencies.add_component_dependencies(reads, writes, false);
			entt_thread_pool->enqueue([this, view, entity_chunk_begin, entity_chunk_end, reads, writes, func]()
				{
					//auto view = registry.view<Components...>(entt::exclude<Excludes...>);
					for (auto entity_iter = entity_chunk_begin; entity_iter != entity_chunk_end; ++entity_iter)
					{
						auto entity = *entity_iter;
						func(entity, view.template get<Reads>(entity)..., view.get<Writes>(entity)...); // needs to match oreder in system
					} // unsure if I need .template ... might need it, might not.
					systems_dependencies.remove_component_dependencies(reads, writes);
				}
			);
		}
		systems_dependencies.remove_component_dependencies(reads, writes);
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
private:
	double delta_time = 0;
	std::shared_ptr<Scene> scene;
	std::shared_ptr<WorkerThreadPool> entt_thread_pool;
	std::shared_ptr<WorkerThreadPool> general_thread_pool;
	entt::registry& registry;
	SystemsContextDependencies systems_dependencies;
	SystemsContextStorage systems_storage;
};