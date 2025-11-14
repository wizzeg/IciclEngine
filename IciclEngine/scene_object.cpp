#include "scene_object.h"

#include "scene.h"
#include "macros.h"


SceneObject::SceneObject(const std::string a_name, std::weak_ptr<Scene> a_scene)
	: name(a_name), scene(a_scene)
{
	component_datas.emplace_back(std::make_unique<NameComponentData>(NameComponent{ a_name }));
}

SceneObject::SceneObject(const std::string a_name, std::weak_ptr<SceneObject> a_parent, std::weak_ptr<Scene> a_scene)
	: name(a_name), parent(a_parent), scene(a_scene)
{
	component_datas.emplace_back(std::make_unique<NameComponentData>(NameComponent{ a_name }));
}

void SceneObject::add_child(std::weak_ptr<SceneObject> a_child)
{
	if (auto a_shared = a_child.lock())
	{
		for (size_t i = 0; i < children.size(); i++)
		{
			if (auto shared = children[i].lock())
			{
				if (a_shared == shared)
				{
					return;
				}
			}
		}
	}
	children.push_back(a_child);
};

void SceneObject::draw_components()
{
	// !entity_handle.valid() -> !!!! this is how you check if entity still exists !!!!!
	///only test, this will not work in actual editor, works only with specific circumstances
	if (!entity_handle.valid())
	{
		PRINTLN("entity null, removing");
	}
	bool remove_component = false;
	for (size_t i = component_datas.size(); i > 0; i--)
	{
		if (!component_datas[i - 1]->draw_imgui(entity_handle)) // has component?
		{
			component_datas.erase(component_datas.begin() + (i -1));
		}
	}
}

entt::handle SceneObject::get_entity_handle() const
{
	return entity_handle;
}

entt::handle SceneObject::to_runtime(std::weak_ptr<Scene> a_scene)
{
	if (auto scene = a_scene.lock())
	{
		entity_handle = scene->create_entity(name);
		for (size_t i = 0; i < component_datas.size(); i++)
		{
			component_datas[i]->to_runtime(entity_handle);
		}
	}
	return entity_handle;
}

void SceneObject::scene_ended()
{

}