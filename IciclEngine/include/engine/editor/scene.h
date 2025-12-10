#pragma once
#include <engine/utilities/entt_modified.h>
#include <vector>

class SceneObject;

class Scene : public std::enable_shared_from_this<Scene>
{
private:
	entt::registry registry;
	std::vector<entt::handle> entity_handles;
	std::vector<std::shared_ptr<SceneObject>> scene_objects;
	std::vector<std::shared_ptr<SceneObject>> root_scene_objects;
	bool runtime = false;
	uint64_t next_index = 0;
public:
	Scene();
	Scene(std::shared_ptr<Scene> a_scene);
	~Scene();

	entt::registry& get_registry() { return registry; }
	entt::handle create_entity(const std::string a_name);
	void destroy_entity(entt::entity a_entity);
	void destroy_scene_object(std::weak_ptr<SceneObject> a_scene_object);

	std::weak_ptr<SceneObject> new_scene_object(const std::string a_name, bool as_root_object);
	//bool add_scene_object(SceneObject& a_scene_object);

	const std::vector<std::shared_ptr<SceneObject>> get_scene_objects();
	const std::vector<std::shared_ptr<SceneObject>> get_root_scene_objects();

	//void draw_imgui();
	void to_runtime();
	bool is_runtime() { return runtime; };
	void stop_scene();
	uint64_t get_next_index() { return next_index++; }
};

