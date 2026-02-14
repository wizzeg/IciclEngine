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

template<typename...T> struct WithWrite {};
template<typename...T> struct WithRead {};
template<typename...T> struct WithOut {};
template<typename...T> struct WithRef {}; // huh... these don't actually work in the general enqueue...
//template<typename...T> struct WithGet {}; // this is kinda required for physics, when you only read

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
			add_write_dependency<Writes...>();
			add_read_dependency<Reads...>();
		}
		return true;
	}

	template<typename... Reads, typename... Writes, typename... Mods>
	bool add(WithRead<Reads...>, WithWrite<Writes...>, WithRef<Mods...>, bool check_dependencies = true)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			if (check_dependencies)
				if (dependency_collision({ typeid(Reads)... }, { typeid(Writes)..., typeid(Mods)... }))
					return false;
			add_write_dependency<Writes..., Mods...>();
			add_read_dependency<Reads...>();
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
			add_read_dependency<Reads...>();
		}
		return true;
	}

	template<typename... Reads, typename... Mods>
	bool add(WithRead<Reads...>, WithRef<Mods...>, bool check_dependencies = true)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			if (check_dependencies)
				if (dependency_collision({ typeid(Reads)... }, { typeid(Mods)... }))
					return false;
			add_write_dependency<Mods...>();
			add_read_dependency<Reads...>();
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
			add_write_dependency<Writes...>();
		}
		return true;
	}

	template<typename... Writes, typename... Mods>
	bool add(WithWrite<Writes...>, WithRef<Mods...>, bool check_dependencies = true)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			if (check_dependencies)
				if (dependency_collision({ }, { typeid(Writes)..., typeid(Mods)... }))
					return false;
			add_write_dependency<Writes..., typeid(Mods)...>();
		}
		return true;
	}




	template<typename... Reads, typename... Writes>
	void remove(WithRead<Reads...>, WithWrite<Writes...>)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			remove_write_dependency<Writes...>();
			remove_read_dependency<Reads...>();
		}
	}

	template<typename... Reads, typename... Writes, typename...Mods>
	void remove(WithRead<Reads...>, WithWrite<Writes...>, WithRef<Mods...>)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			remove_write_dependency<Writes..., Mods...>();
			remove_read_dependency<Reads...>();
		}
	}

	template<typename... Reads>
	void remove(WithRead<Reads...>)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			remove_read_dependency<Reads...>();
		}
	}

	template<typename... Reads, typename... Mods>
	void remove(WithRead<Reads...>, WithRef<Mods...>)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			remove_write_dependency<Mods...>();
			remove_read_dependency<Reads...>();
		}
	}


	template<typename... Writes>
	void remove(WithWrite<Writes...>)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			remove_write_dependency<Writes...>();
		}
	}

	template<typename... Writes, typename... Mods>
	void remove(WithWrite<Writes...>, WithRef<Mods...>)
	{
		{
			std::lock_guard comp_lock(components_mutex);
			remove_write_dependency<Writes..., Mods...>();
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

	template<typename... Reads>
	void add_read_dependency()
	{
		//std::type_index type = typeid(Read);
		std::vector<std::type_index> types = { typeid(Reads)... };
		std::sort(types.begin(), types.end());
		size_t insertion_index = 0;
		for (auto& type : types)
		{
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

		// change to better insert
		//std::vector<std::type_index> merged;
		//merged.reserve(reading_components.size() + types.size());

		//auto it_rc = reading_components.begin();
		//auto it_t = types.begin();

		//while (it_rc != reading_components.end() &&
		//	it_t != types.end())
		//{
		//	if (*it_t < *it_rc)
		//	{
		//		merged.push_back(*it_t++);
		//	}
		//	else
		//	{
		//		merged.push_back(*it_rc++);
		//	}
		//}

		//merged.insert(merged.end(), it_t, types.end());
		//merged.insert(merged.end(), it_rc, reading_components.end());

		//reading_components.swap(merged);
	}
	template<typename... Writes>
	void add_write_dependency()
	{
		//std::type_index type = typeid(Write);
		std::vector<std::type_index> types = { typeid(Writes)... };
		std::sort(types.begin(), types.end());
		size_t insertion_index = 0;
		for (auto& type : types)
		{
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
	}

	template<typename... Reads>
	void remove_read_dependency()
	{
		//std::type_index type = typeid(Read);
		std::vector<std::type_index> types = { typeid(Reads)... };
		std::sort(types.begin(), types.end());
		size_t removal_index = 0;
		for (auto& type : types)
		{
			for (; removal_index < reading_components.size(); ++removal_index)
			{
				auto& read_comp = reading_components[removal_index];
				if (type == read_comp)
				{
					reading_components.erase(reading_components.begin() + removal_index); // not great..
					break;
				}
			}
		}

		// change this to a std::remove_if
		//auto erase_end = std::remove_if(reading_components.begin(), reading_components.end(),
		//	[&](const auto& read_comp)
		//	{
		//		if (removal_index >= types.size())
		//			return false;

		//		while (removal_index < types.size() && read_comp > types[removal_index])
		//			++removal_index;
		//			
		//		if (read_comp == types[removal_index])
		//		{
		//			++removal_index;
		//			return true;
		//		}
		//		return false;
		//	});
		//reading_components.erase(erase_end, reading_components.end());

	}
	template<typename... Writes>
	void remove_write_dependency()
	{

		std::vector<std::type_index> types = { typeid(Writes)... };
		std::sort(types.begin(), types.end());
		size_t removal_index = 0;
		for (auto& type : types)
		{
			for (; removal_index < writing_components.size(); ++removal_index)
			{
				auto& write_comp = writing_components[removal_index];
				if (type == write_comp)
				{
					writing_components.erase(writing_components.begin() + removal_index);
					break;
				}
			}
		}
	}
};