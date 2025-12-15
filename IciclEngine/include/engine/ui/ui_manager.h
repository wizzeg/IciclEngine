#pragma once

#include <memory>
#include <engine/ui/ui_scene_hierarchy_drawer.h>
#include <engine/ui/ui_object_property_drawer.h>

class Scene;
class SceneObject;

class UIManager : UISceneHierarchyDrawer , UIObjectPropertyDrawer
{
	bool should_draw_object_properties = true;
	bool shoud_draw_object_hierarchy = true;
	std::weak_ptr<Scene> scene;
	std::weak_ptr<SceneObject> prev_selected_scene_object;
	//UISceneHierarchyDrawer ui_hiearchy_drawer;
	//UIObjectPropertyDrawer ui_property_drawer;
public:
	void draw_object_hierarchy();
	void draw_object_properties();
	void set_scene(std::weak_ptr<Scene> a_scene);
};

