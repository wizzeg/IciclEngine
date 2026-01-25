#pragma once
#include <unordered_map>
#include <string>
#include <engine/game/systems.h>
#include <functional>
#include <memory>
#include <vector>
// first create the registry

struct SystemsRegistry
{
	static SystemsRegistry& instance()
	{
		static SystemsRegistry instance;
		return instance;
	}

	void register_system(const std::string& a_name)
	{
		for (auto& name : system_names)
		{
			if (a_name == name)
			{
				return;
			}
		}
		system_names.push_back(a_name);
	}

	template<typename System>
	const std::string& get_name()
	{
		static std::string name = std::string(typeid(System).name());
		if (name.rfind("struct ", 0) == 0) {
			name.erase(0, 7);
		}
		else if (name.rfind("class ", 0) == 0) {
			name.erase(0, 6);
		}
		else if (name.rfind("const ", 0) == 0) {
			name.erase(0, 6);
		}
		return name;
	}

	template<typename System>
	const std::string& get_name(System* a_system)
	{
		static std::string name = std::string(typeid(decltype(*a_system)).name());
		if (name.rfind("struct ", 0) == 0) {
			name.erase(0, 7);
		}
		else if (name.rfind("class ", 0) == 0) {
			name.erase(0, 6);
		}
		return name;
	}

	std::vector<std::string> get_all_system_names()
	{
		return system_names;
	}

	std::vector<std::string> system_names; // so can use this name to then use in factory to get the system
	SystemsRegistry() = default;
};

struct SystemsFactory
{
	static SystemsFactory& instance()
	{
		static SystemsFactory instance;
		return instance;
	}

	template<typename System>
	void register_factory(const std::string& a_name)
	{
		static_assert(std::is_base_of_v<SystemBase, System>, "System must inherit SystemBase");
		name_to_system[a_name] = []()
			{
				return std::make_shared<System>();
			};
	}

	bool has_factory(const std::string& a_name)
	{
		auto it = name_to_system.find(a_name);
		if (it != name_to_system.end())
			return true;
		return false;
	}

	std::shared_ptr<SystemBase> create_system(const std::string& a_name)
	{
		auto it = name_to_system.find(a_name);
		if (it != name_to_system.end())
			return it->second();
		return nullptr;
	}
	std::unordered_map<std::string, std::function<std::shared_ptr<SystemBase>()>> name_to_system;
};

#define REGISTER_SYSTEM(SystemType) \
	namespace { \
        struct _systems_##SystemType { \
			_systems_##SystemType() \
			{ \
				SystemsRegistry::instance().register_system(\
				SystemsRegistry::instance().get_name<SystemType>()); \
				SystemsFactory::instance().register_factory<SystemType>(\
				SystemsRegistry::instance().get_name<SystemType>()); \
			} \
		}; \
		static _systems_##SystemType system_##SystemType; \
	}