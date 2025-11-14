#pragma once
#include <entt/entt.hpp>
#include <vector>
#include "scene_object.h"

class Entity;

class Scene : public std::enable_shared_from_this<Scene>
{
private:
	entt::registry registry;
	std::vector<std::shared_ptr<Entity>> entities;
	std::vector<std::shared_ptr<SceneObject>> scene_objects;

public:
	Scene();
	Scene(std::shared_ptr<Scene> a_scene);
	~Scene();

	//template<typename T, typename... Args>
	//Entity create_entity(Args&&... args)
	//{
	//	entities.emplace_back(std::make_shared<Entity>(Entity(shared_from_this())));
	//	auto& entity = entities.back();
	//	(entity->add_component(std::forward<Args>(args)),...) // too complicated... how do I even get the args? can't, because they'd be retrieved dynamically
	//	return this;
	//}

	Entity& create_entity(const std::string a_name, int trash);

	entt::registry& get_registry() { return registry; }
	entt::handle create_entity(const std::string a_name, bool a_add_to_registry = false);

	std::weak_ptr<SceneObject> new_scene_object(const std::string a_name);

	const std::vector<std::shared_ptr<SceneObject>> get_scene_objects();

	void draw_imgui();
	//friend class Entity;
};

