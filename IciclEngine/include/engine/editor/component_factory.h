#pragma once
#include <engine/editor/scene_object.h>

struct ComponentFactory
{
	static ComponentFactory& instance()
	{
		static ComponentFactory instance;
		return instance;
	}

	template<typename TComponent>
	void register_factory(const std::string& a_comp_name)
	{
		component_factories[a_comp_name] = [](SceneObject* a_scene_obj_ptr)
			{
				a_scene_obj_ptr->add_component<TComponent>();
			};
	}

	void create_component(const std::string a_comp_name, SceneObject* a_scene_obj_ptr)
	{
		auto it = component_factories.find(a_comp_name);
		if (it != component_factories.end())
		{
			it->second(a_scene_obj_ptr);
		}
	}

private:
	// we store a function which takes in a scene object
	std::unordered_map<std::string, std::function<void(SceneObject*)>> component_factories;
	// really I should just return the component? hmm, but I guess I don't know what the component is, so makes ense
};