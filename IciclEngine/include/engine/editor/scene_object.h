#pragma once

#include <string>
#include <entt/entt.hpp>
#include <engine/game/component_data.h>
#include <engine/utilities/macros.h>

class Scene;

class SceneObject
{

private:

	std::string name;
	std::weak_ptr<SceneObject> parent;
	std::vector<std::weak_ptr<SceneObject>> children;
	std::weak_ptr<Scene> scene;
	
	std::vector<std::unique_ptr<ComponentDataBase>> component_datas;
	std::vector<std::type_index> component_types;

	entt::handle entity_handle;

	bool runtime = false;
	bool ui_opened = false;
	bool original = true;

public:
	SceneObject() { name = "none"; }
	SceneObject(const std::string a_name, std::weak_ptr<Scene> a_scene); ///////////////////// REMOVE USAGE OF ENTITY (use entt::handle)
	SceneObject(const std::string a_name, std::weak_ptr<SceneObject> a_parent, std::weak_ptr<Scene> a_scene);///////////////////// REMOVE USAGE OF ENTITY (use entt::handle)
	~SceneObject() { PRINTLN("DECONSTRUCTOR CALLED"); }// if this has parent, add all children to it, otherwise I don't know... 
	void scene_ended();
	entt::entity get_entity() { return entity_handle.entity(); }
	bool has_valid_entity() { return entity_handle.valid(); }

	entt::handle get_entity_handle() const; ///////////////////// REMOVE USAGE OF ENTITY (use entt::handle)

	void add_child(std::weak_ptr<SceneObject> a_child);

	template <typename TcomponentData, typename TComponent>
	bool add_component_data(TComponent&& a_component)
	{
		static_assert(std::is_base_of<ComponentData<std::decay_t<TComponent>>, TcomponentData>::value, "TcomponentData must derive from ComponentData<T> with matching TComponent");
		static_assert(std::is_standard_layout<std::decay_t<TComponent>>::value, "TComponent must be a standard-layout type (e.g no inheritance)");
		static_assert(std::is_trivial<std::decay_t<TComponent>>::value, "TComponent must be trivial (e.g. no smart pointers)");
		std::type_index t_type = typeid(std::decay_t<TComponent>);
		for (size_t i = 0; i < component_types.size(); i++)
		{
			if (t_type == component_types[i])
			{
				return false;
			}
		}
		component_types.push_back(t_type);
		component_datas.emplace_back(std::make_unique<TcomponentData>(std::forward<TComponent>(a_component)));
		return true;
	}

	template <typename TComponent>
	bool replace_component_data(TComponent&& a_component)
	{
		static_assert(std::is_standard_layout<std::decay_t<TComponent>>::value, "TComponent must be a standard-layout type (e.g no inheritance)");
		static_assert(std::is_trivial<std::decay_t<TComponent>>::value, "TComponent must be trivial (e.g. no smart pointers)");
		std::type_index t_type = typeid(std::decay_t<TComponent>);
		bool error = false;
		for (size_t i = 0; i < component_types.size(); i++) // may want to consider to sort these in order of each other.
		{
			if (t_type == component_types[i])
			{
				for (size_t i = 0; i < component_datas.size(); i++)
				{
					if (component_datas[i].get()->get_type() == t_type)
					{
						ComponentData<std::decay_t<TComponent>>& component_data = static_cast<ComponentData<std::decay_t<TComponent>>&>(*component_datas[i]);
						std::decay_t<TComponent> component = std::forward<TComponent>(a_component);
						component_data.set_new_component_value(component);
						return true;
					}
				}
				error = true;
				break;
			}
		}
		if (error)
		{
			PRINTLN("MISSMATCH: scene_object has component_type info of component for which there's no comopnent_data for");
			// perhaps remove the component_type ?
		}
		return false;
	}

	template <typename TComopnentData, typename TComponent>
	bool add_or_replace_component_data(TComponent&& a_component)
	{
		if (!replace_component_data<std::decay_t<TComponent>>(std::forward<TComponent>(a_component)))
		{
			if (!add_component_data<TComopnentData>(std::forward<TComponent>(a_component)))
			{
				PRINTLN("ERROR: somehow component could not be replaced, and not added");
				return false;
			}
			return true;
		}
		return false;
	}


	template<typename TComponent>
	bool get_component(TComponent*& a_component)
	{
		static_assert(std::is_standard_layout<std::decay_t<TComponent>>::value, "TComponent must be a standard-layout type (e.g no inheritance)");
		static_assert(std::is_trivial<std::decay_t<TComponent>>::value, "TComponent must be trivial (e.g. no smart pointers)");
		std::type_index t_type = typeid(std::decay_t<TComponent>);
		bool error = false;
		for (size_t i = 0; i < component_types.size(); i++) // may want to consider to sort these in order of each other.
		{
			if (t_type == component_types[i])
			{
				for (size_t i = 0; i < component_datas.size(); i++)
				{
					if (component_datas[i].get()->get_type() == t_type)
					{
						ComponentData<std::decay_t<TComponent>>& component_data = static_cast<ComponentData<std::decay_t<TComponent>>&>(*component_datas[i]);
						a_component = &component_data.get_component();
						return true;
					}
				}
				error = true;
				break;
			}
		}
		if (error)
		{
			PRINTLN("MISSMATCH: scene_object has component_type info of component for which there's no comopnent_data for");
			// perhaps remove the component_type ?
		}
		return false;
	}

	template<typename TComponent>
	bool remove_component_data()
	{

		std::type_index t_type = typeid(TComponent);
		size_t index = 0;
		bool found_index = false;
		bool found_component = false;
		for (size_t i = 0; i < component_types.size(); i++) // may want to consider to sort these in order of each other.
		{
			if (t_type == component_types[i])
			{
				index = i;
				found_index = true;
				for (size_t i = 0; i < component_datas.size(); i++)
				{
					if (component_datas[i].get()->get_type() == t_type)
					{
						component_datas.erase(component_datas.begin() + i);
						found_component = true;
						break;
					}
				}
				break;
			}
		}
		if (found_component != found_index)
		{
			PRINTLN("MISSMATCH: scene_object has component_type info of component for which there's no comopnent_data for");
			// perhaps remove the component_type ?
		}
		if (found_index)
		{
			component_types.erase(component_types.begin() + index);
		}
		return found_component;
	}

	entt::handle to_runtime(std::weak_ptr<Scene> a_scene);
	//void draw_components();

	size_t num_children() const { return children.size(); };
	std::string get_name() const { return name; };

	std::vector<std::weak_ptr<SceneObject>> get_children() { return children; };

	const std::vector<std::unique_ptr<ComponentDataBase>>& get_component_datas() const { return component_datas; }
	bool is_runtime() const { return runtime; }
};

