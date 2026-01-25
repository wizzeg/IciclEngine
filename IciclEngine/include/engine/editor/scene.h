#pragma once
#include <engine/utilities/entt_modified.h>
#include <vector>
#include <engine/editor/scene_object.h>

class SceneObject;
struct SystemBase;

class Scene : public std::enable_shared_from_this<Scene>
{
private:
	entt::registry registry;
	std::string name;
	std::vector<entt::handle> entity_handles;
	std::vector<std::shared_ptr<SceneObject>> scene_objects;
	std::vector<std::shared_ptr<SceneObject>> root_scene_objects;
	std::vector<std::shared_ptr<SystemBase>> systems;
	bool runtime = false;
	uint64_t next_index = 0;
public:
	Scene();
	Scene(const std::string& a_path);
	Scene(std::shared_ptr<Scene> a_scene);
	~Scene();

	void reset();

	bool save(std::string a_path);
	bool load(std::string a_path, bool clear_registry = true);

	entt::registry& get_registry() { return registry; }
	entt::handle create_entity(const std::string a_name);
	void destroy_entity(entt::entity a_entity);
	void destroy_scene_object(std::weak_ptr<SceneObject> a_scene_object);
	void add_scene_object(std::shared_ptr<SceneObject> a_scene_object); // use only on load
	void parent_scene_object(std::weak_ptr<SceneObject> a_parent_scene_object, std::weak_ptr<SceneObject> a_target_scene_object);
	void orphan_scene_object(std::weak_ptr<SceneObject> a_target_scene_object);
	std::weak_ptr<SceneObject> get_scene_object_by_ID(uint32_t a_id);

	std::weak_ptr<SceneObject> new_scene_object(const std::string a_name, bool as_root_object);
	//bool add_scene_object(SceneObject& a_scene_object);

	const std::vector<std::shared_ptr<SceneObject>> get_scene_objects();
	const std::vector<std::shared_ptr<SceneObject>> get_root_scene_objects();

	//void draw_imgui();
	void start_runtime();
	bool is_runtime() { return runtime; };
	void stop_runtime();
	uint64_t get_next_index() { return next_index++; }

	void add_system(std::shared_ptr<SystemBase> a_system);
	void remove_system(std::weak_ptr<SystemBase> a_system);
	void reorder_systems();
	std::vector<std::shared_ptr<SystemBase>>& get_systems();
};

