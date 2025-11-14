#include "scene.h"

#include "components.h"
#include "scene_object.h"
#include "imgui-docking/imgui.h"

Scene::Scene()
{
	
}
Scene::Scene(std::shared_ptr<Scene> a_scene)
{
	auto scene = a_scene.get();
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
		scene_objects[i].get()->scene_ended();
	}
}

entt::handle Scene::create_entity(const std::string a_name)
{
	auto entity = registry.create();
	auto& handle = entity_handles.emplace_back(entt::handle{ registry, entity });
	//registry.emplace<NameComponent>(entity, "a_name");
	return handle;
}

std::weak_ptr<SceneObject> Scene::new_scene_object(const std::string a_name)
{
	//auto scene_object = SceneObject(a_name, shared_from_this());
	//std::weak_ptr<SceneObject> scene_object = scene_objects.emplace_back(std::make_shared<SceneObject>(a_name, shared_from_this()));
	//return scene_object;
	return scene_objects.emplace_back(std::make_shared<SceneObject>(a_name, shared_from_this()));
}

const std::vector<std::shared_ptr<SceneObject>> Scene::get_scene_objects()
{
	return scene_objects;
}

void Scene::draw_imgui()
{
	for (size_t i = scene_objects.size(); i > 0; i--)
	{
		if (!scene_objects[i-1].get()->has_valid_entity())
		{
			scene_objects.erase(scene_objects.begin() + (i-1));
			PRINTLN("removing scene_object, as it has no entity");
		}
		else
		{
			scene_objects[i - 1].get()->draw_components();
		}
		
	}
}

void Scene::to_runtime()
{
	for (size_t i = 0; i < scene_objects.size(); i++)
	{
		scene_objects[i].get()->to_runtime(shared_from_this());
	}
}