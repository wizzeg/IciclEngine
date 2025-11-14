#pragma once
#include <entt/entt.hpp>
#include "scene.h"

class Entity // this class is kinda pointless tbh
{
private:
	entt::handle entity_handle;
	std::weak_ptr<Scene> scene;
	std::string name;

	// should have a list of IciclObjects (c++ scripts/classes), add this list to a component to the entt::entity, then a system calls update on all of them.
	// we need a sceneobject to instantiate Entity, and hold stuff like TransformComponentData which is the data that can be selected in the editor.
	// the TransformComponentData inherits IComponentData which has methods to instatiante and write data, and also to be displayed by imgui

public:
	Entity();
	Entity(std::weak_ptr<Scene> a_scene);
	Entity(entt::entity a_entity, std::weak_ptr<Scene> a_scene);
	~Entity();
	void set_name(std::string&& a_name) { name = std::move(a_name); }
	void set_name(const std::string& a_name) { name = a_name; }
	entt::handle get_handle() const { return entity_handle; }
	void set_handle(entt::handle a_handle) { entity_handle = a_handle; }

	template<typename T, typename... Args>
	T& add_component(Args&&... args)
	{
		if (entity_handle != entt::null)
		{
			if (auto shared = scene.lock())
			{
				return shared->get_registry().emplace<T>(entity_handle, std::forward<Args>(args)...);
			}
		}
		static T default_T{};
		return default_T;

	}


	template<typename T>
	T& get_component()
	{
		if (entity_handle != entt::null)
		{
			if (auto shared = scene.lock())
			{
				return shared->get_registry().get<T>(entity_handle);
			}
		}
		static T default_T{};
		return default_T;
		
	}


	template<typename T>
	void remove_component()
	{
		if (entity_handle != entt::null)
		{
			if (auto lock = scene.lock())
			{
				lock->get_registry().remove<T>(entity_handle);
			}
		}
	}
};

