//#include "scene.h"
#include <engine/editor/scene.h>

#include <engine/game/components.h>
#include <engine/editor/scene_object.h>
#include <imgui-docking/imgui.h>

Scene::Scene()
{
	
}
Scene::Scene(std::shared_ptr<Scene> a_scene) // do I ever use this??
{
	auto scene = a_scene;
	auto scene_objects = scene->get_scene_objects();
	for (size_t i = 0; i < scene_objects.size(); i++)
	{
		scene_objects[i]->to_runtime(shared_from_this());
	}
}
Scene::~Scene()
{
	for (size_t i = 0; i < scene_objects.size(); i++)
	{
		scene_objects[i]->scene_ended();
	}
	
}

entt::handle Scene::create_entity(const std::string a_name)
{
	auto entity = registry.create();
	auto& handle = entity_handles.emplace_back(entt::handle{ registry, entity });
	//registry.emplace<NameComponent>(entity, "a_name");
	return handle;
}

void Scene::destroy_entity(entt::entity a_entity)
{
	if (runtime)
	{
		uint32_t ent = (uint32_t)a_entity;
		if (registry.valid(a_entity))
		{
			registry.destroy(a_entity);
		}
	}
}

void Scene::destroy_scene_object(std::weak_ptr<SceneObject> a_scene_object)
{
	if (auto scene_object = a_scene_object.lock())
	{
		if (runtime)
		{
			destroy_entity(scene_object->get_entity());
		}
		auto children = scene_object->get_children();
		for (size_t i = 0; i < children.size(); i++)
		{
			destroy_scene_object(children[i]);
		}
		for (size_t i = 0; i < scene_objects.size(); i++)
		{
			if (scene_object.get() == scene_objects[i].get())
			{
				scene_objects.erase(scene_objects.begin() + i);
				break;
			}

		}
		for (size_t i = 0; i < root_scene_objects.size(); i++)
		{
			if (scene_object.get() == root_scene_objects[i].get())
			{
				root_scene_objects.erase(root_scene_objects.begin() + i);
				break;
			}
		}
	}

}

std::weak_ptr<SceneObject> Scene::new_scene_object(const std::string a_name, bool as_root_object)
{
	//auto scene_object = SceneObject(a_name, shared_from_this());
	//std::weak_ptr<SceneObject> scene_object = scene_objects.emplace_back(std::make_shared<SceneObject>(a_name, shared_from_this()));
	//return scene_object;
	auto scene_object = std::make_shared<SceneObject>(a_name, shared_from_this());
	if (as_root_object)
	{
		root_scene_objects.push_back(scene_object);
	}
	scene_objects.push_back(scene_object);
	if (runtime)
	{
		scene_object->to_runtime(shared_from_this());
	}
	return scene_object;
}

//bool Scene::add_scene_object(SceneObject& a_scene_object)
//{
//	root_scene_objects.push_back(std::make_shared<SceneObject>(a_scene_object));
//	return true;
//}

const std::vector<std::shared_ptr<SceneObject>> Scene::get_scene_objects()
{
	return scene_objects;
}
const std::vector<std::shared_ptr<SceneObject>> Scene::get_root_scene_objects()
{
	return root_scene_objects;
}

//void Scene::draw_imgui()
//{
//	for (size_t i = scene_objects.size(); i > 0; i--)
//	{
//		if (runtime && !scene_objects[i-1].get()->has_valid_entity())
//		{
//			scene_objects.erase(scene_objects.begin() + (i-1));
//			PRINTLN("removing scene_object, as it has no entity");
//		}
//		else
//		{
//			//if (ImGui::TreeNode(scene_objects[i - 1].get()->get_name().c_str()))
//			//{
//				scene_objects[i - 1].get()->draw_components();
//			//}
//		}
//	}
//}

void Scene::to_runtime()
{
	runtime = true;
	for (size_t i = 0; i < scene_objects.size(); i++)
	{
		scene_objects[i]->to_runtime(shared_from_this());
	}
}

void Scene::stop_scene()
{
	runtime = false;
	registry.clear();
}