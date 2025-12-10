#pragma once
#include <functional>
#include <string>
#include <typeindex>
#include <engine/game/field_info.h>
#include <unordered_map>

using FieldInfoGenerator = std::function<std::vector<FieldInfo>(void*)>;
struct ComponentRegistryData
{
	std::string comp_name;
	std::vector<std::string> categories;
	std::type_index comp_type;
	FieldInfoGenerator field_info_generator;
};

struct ComponentRegistry
{
	static ComponentRegistry& instance()
	{
		static ComponentRegistry instance;
		return instance;
	}

	template<typename TComponent>
	void register_component(const std::string& a_name, const std::vector<std::string> a_categories, FieldInfoGenerator a_field_info_gen)
	{
		ComponentRegistryData comp_reg;
		comp_reg.comp_name = a_name;
		comp_reg.categories = a_categories;
		comp_reg.comp_type = std::type_index(typeid(TComponent));
		comp_reg.field_info_generator = a_field_info_gen;

		components_by_name[comp_reg.comp_name] = comp_reg.comp_type;
		component_registry_by_comp_type[comp_reg.comp_type] = comp_reg;

		for (const auto& category : comp_reg.categories)
		{
			components_matching_category[category].push_back(comp_reg.comp_name);
		}
			
	}

	template <typename TComponent>
	std::vector<FieldInfo> get_field_info(TComponent* component)
	{
		auto it = component_registry_by_comp_type.find(std::type_index(typeid(TComponent)));
		if (it != component_registry_by_comp_type.end() && it->second.field_info_generator)
		{
			return it->second.field_info_generator;
		}
	}

	template <typename TComponent>
	std::vector<FieldInfo> get_component_name(TComponent* component)
	{
		auto it = component_registry_by_comp_type.find(std::type_index(typeid(TComponent)));
		if (it != component_registry_by_comp_type.end())
		{
			return it->second.comp_name;
		}
	}
	template <typename TComponent>
	std::vector<FieldInfo> get_component_name(TComponent* component)
	{
		auto it = component_registry_by_comp_type.find(std::type_index(typeid(TComponent)));
		if (it != component_registry_by_comp_type.end())
		{
			return it->second.comp_name;
		}
	}
	template <typename TComponent>
	std::vector<std::string> get_all_component_names()
	{
		std::vector<std::string> names;
		for (const auto& [name, type] : components_by_name)
		{
			names.push_back(name);
		}
		return names;
	}

	std::vector<std::string> get_all_categories() const
	{
		std::vector<std::string> categories;
		for (const auto& [category, name] : component_names_matching_category)
		{
			categories.push_back(category);
		}
		return categories;
	}

	std::vector<std::string> get_component_categories(const std::string& a_component_name) const
	{
		auto type_it = components_by_name.find(a_component_name);
		if (type_it != components_by_name.end())
		{
			auto reg_it = component_registry_by_comp_type.find(type_it->second);
			if (reg_it != component_registry_by_comp_type.end())
			{
				return reg_it->second.categories;
			}
		}
		return {};
	}

	std::vector<std::string> get_components_matching_categories(const std::vector<std::string>& a_requested_categories) const
	{
		std::vector<std::string> matching;
		for (const auto& [name, type] : components_by_name)
		{
			auto reg_it = component_registry_by_comp_type.find(type);
			if (reg_it != component_registry_by_comp_type.end())
			{
				const auto& comp_categories = reg_it->second.categories;
				for (const auto& requested_category : a_requested_categories)
				{
					if (std::find(comp_categories.begin(), comp_categories.end(), requested_category) != comp_categories.end())
					{
						matching.push_back(name);
						break;
					}
				}
			}
		}
		return matching;
	}

private:
	ComponentRegistry() = default;
	std::unordered_map<std::type_index, ComponentRegistryData> component_registry_by_comp_type;
	std::unordered_map<std::string, std::type_index> components_by_name;
	std::unordered_map<std::string, std::vector<std::string>> component_names_matching_category;
};

