#include "scene_object.h"
#include "scene.h"
#include "entity.h"
#include "macros.h"


SceneObject::SceneObject(const std::string a_name, std::weak_ptr<Scene> a_scene, std::shared_ptr<Entity> a_entity)
	: name(a_name), scene(a_scene), entity(a_entity) { }

SceneObject::SceneObject(const std::string a_name, std::weak_ptr<SceneObject> a_parent, std::weak_ptr<Scene> a_scene, std::shared_ptr<Entity> a_entity)
	: name(a_name), parent(a_parent), scene(a_scene), entity(a_entity) { }

void SceneObject::add_child(std::weak_ptr<SceneObject> a_child)
{
	if (auto a_lock = a_child.lock())
	{
		for (size_t i = 0; i < children.size(); i++)
		{
			if (auto lock = children[i].lock())
			{
				if (a_lock == lock)
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
	for (size_t i = 0; i < component_datas.size(); i++)
	{
		component_datas[i]->draw_imgui();
	}
}

const std::weak_ptr<Entity> SceneObject::get_entity()
{
	return entity;
}

void SceneObject::to_runtime(std::weak_ptr<Scene> a_scene)
{
	if (auto scene = a_scene.lock())
	{
		auto& entity = scene->create_entity("test", 3);
		for (size_t i = 0; i < component_datas.size(); i++)
		{
			component_datas[i]->to_runtime(entity.get_handle());
		}
	}
}