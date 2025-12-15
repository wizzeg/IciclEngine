//#include "scene.h"
#include <engine/editor/scene.h>

#include <engine/game/components.h>
#include <engine/editor/scene_object.h>
#include <imgui-docking/imgui.h>
#include <fstream>

Scene::Scene()
{
	name = "new scene";
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

bool Scene::save(std::string a_path)
{
	json j = json::object();
	j["name"] = name;
	j["next index"] = next_index;
	j["root objects"] = json::array();
	for (auto& scene_object : root_scene_objects)
	{
		j["root objects"].push_back(scene_object->save());
	}

	std::ofstream file(a_path);
	if (!file)
	{
		PRINTLN("Failed to save at: {}", a_path);
		return false;
	}

	file << j.dump(4);  // Pretty-print with 4-space indent
	return true;
}

bool Scene::load(std::string a_path, bool clear_registry)
{
	std::ifstream file(a_path);
	if (!file)
	{
		PRINTLN("Failed to load at: {}", a_path);
		return false;
	}

	if (clear_registry)
	{
		registry.clear();
		for (auto [entity, name_comp] : registry.view<NameComponent>().each())
		{
			PRINTLN("ENTITIES STILL HERE");
		}
	}

	json j;
	file >> j;
	root_scene_objects.clear();
	scene_objects.clear();
	j["name"] = name;
	j["next index"] = next_index;

	if (j["root objects"].is_array())
	{
		for (auto& root_obj : j["root objects"])
		{
			root_scene_objects.push_back(SceneObject::load(root_obj, shared_from_this()));
			//scene_objects.push_back(root_scene_objects.back());
		}
	}

	if (runtime)
	{
		for (auto& scene_object : scene_objects)
		{
			scene_object->to_runtime(shared_from_this());
		}
	}

	return true;
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

void Scene::add_scene_object(std::shared_ptr<SceneObject> a_scene_object)
{
	scene_objects.push_back(a_scene_object);
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
	for (auto scene_object : root_scene_objects)
	{
		scene_object->to_runtime(shared_from_this());
	}
	//for (size_t i = 0; i < scene_objects.size(); i++)
	//{
	//	scene_objects[i]->to_runtime(shared_from_this());
	//}
}

void Scene::stop_scene()
{
	runtime = false;
	registry.clear();
}