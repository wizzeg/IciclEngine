#pragma once

#include <string>
#include <entt/entt.hpp>
#include "component_data.h"
#include "macros.h"

class Scene;
class Entity;

class SceneObject
{

private:
	std::string name;
	std::weak_ptr<SceneObject> parent;
	std::vector<std::weak_ptr<SceneObject>> children;
	std::weak_ptr<Scene> scene;
	
	std::vector<std::unique_ptr<ComponentData>> component_datas;

	entt::handle entity_handle;

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

	template <typename T, typename... Args>
	bool add_component_data(Args&&... args)
	{
		static_assert(std::is_base_of<ComponentData, T>::value, "SceneObject: added component data type T must derive from CompnentData");
		//I really should check so that this does not contain one of T already
		for (size_t i = 0; i < component_datas.size(); i++)
		{
			if (typeid(*component_datas[i]) == typeid(T))
			{
				return false;
			}
		}
		component_datas.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
		return true;
	}

	template <typename T, typename... Args>
	bool replace_component_data(Args&&... args)
	{
		static_assert(std::is_base_of<ComponentData, T>::value, "SceneObject: added component data type T must derive from CompnentData");
		//I really should check so that this does not contain one of T already
		for (size_t i = 0; i < component_datas.size(); i++)
		{
			if (typeid(*component_datas[i]) == typeid(T))
			{
				component_datas[i] = std::make_unique<T>(std::forward<Args>(args)...);
				return true;
			}
		}
		//component_datas.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
		return false;
	}

	template <typename T, typename... Args>
	void add_or_replace_component_data(Args&&... args)
	{
		static_assert(std::is_base_of<ComponentData, T>::value, "SceneObject: added component data type T must derive from CompnentData");
		//I really should check so that this does not contain one of T already
		for (size_t i = 0; i < component_datas.size(); i++)
		{
			if (typeid(*component_datas[i]) == typeid(T))
			{
				component_datas[i] = std::make_unique<T>(std::forward<Args>(args)...);
				return;
			}
		}
		component_datas.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
	}


	template<typename Tcomponent, typename Tdata>
	void get_component(Tcomponent& a_component)
	{
		PRINTLN("tried to get component");
		static_assert(std::is_base_of<ComponentData, Tdata>::value, "SceneObject: requested component data type T must derive from ComponentData");
		for (size_t i = 0; i < component_datas.size(); i++)
		{
			if (typeid(*component_datas[i]) == typeid(Tdata))
			{
				if (auto component = dynamic_cast<Tdata*>(component_datas[i].get()))
				{
					a_component = component->name_component;
					return;
				}
			}
		}
	}

	template<typename Tdata>
	bool remove_component_data()
	{
		for (size_t i = 0; i < component_datas.size(); i++)
		{
			if (typeid(*component_datas[i]) == typeid(Tdata))
			{
				component_datas.erase(component_datas.begin() + i);
				return true;
			}
		}
		return false;
	}

	entt::handle to_runtime(std::weak_ptr<Scene> a_scene);
	void draw_components();
};

