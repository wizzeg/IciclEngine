#pragma once
#include <memory>

class SceneObject;

struct UISceneHierarchyDrawer
{
	std::weak_ptr<SceneObject> selected_scene_object;
	void draw_hierarchy_node(std::weak_ptr<SceneObject> a_scene_object, size_t a_id);

	bool open_load_popup = false;
	bool open_save_popup = false;
	char prefab_path[260] = "prefabs/";
};

