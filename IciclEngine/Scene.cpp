#include "scene.h"

#include "entity.h"
#include "components.h"
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

}

Entity& Scene::create_entity(const std::string a_name, int trash)
{
	entities.emplace_back(std::make_shared<Entity>(Entity(shared_from_this())));
	auto& entity = entities.back();
	return *entity;
}


// deprecated, should not be used
entt::handle Scene::create_entity(const std::string a_name, bool a_add_to_registry) // not going to use it like this I tnk...or maybe for "instantiate"
{
	if (a_add_to_registry) // this should really always be true.. I changed my mind on architecture after all...
	{
		entities.emplace_back(std::make_shared<Entity>(registry.create(), shared_from_this()));
		auto& entity = *entities.back().get();
		entity.add_component<NameComponent>(a_name);
		entity.set_name(a_name);
		return entity.get_handle();
	}
	return entt::handle{ registry, entt::null };
}

std::weak_ptr<SceneObject> Scene::new_scene_object(const std::string a_name)
{
	auto& entity = entities.emplace_back(std::make_shared<Entity>(registry.create(), shared_from_this()));
	//auto& entity = *entities.back().get();
	entity->add_component<NameComponent>(a_name);
	entity->set_name(a_name);
	return scene_objects.emplace_back(std::make_shared<SceneObject>(a_name, shared_from_this(), entity));
	//return scene_objects.back();
}

const std::vector<std::shared_ptr<SceneObject>> Scene::get_scene_objects()
{
	return scene_objects;
}

void Scene::draw_imgui()
{
	//for (auto [entity, name] : registry.view<NameComponent>().each())
	//{
	//	ImGui::Text(name.name.c_str());
	//}
	for (size_t i = 0; i < scene_objects.size(); i++)
	{
		scene_objects[i].get()->draw_components();
	}
}