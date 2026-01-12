#pragma once

#include <memory>
#include <engine/ui/ui_scene_hierarchy_drawer.h>
#include <engine/ui/ui_object_property_drawer.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

class Scene;
class SceneObject;

class UIManager : UISceneHierarchyDrawer , UIObjectPropertyDrawer
{
	bool should_draw_object_properties = true;
	bool shoud_draw_object_hierarchy = true;
	bool draw_ui = true;
	std::weak_ptr<Scene> scene;
	std::weak_ptr<SceneObject> prev_selected_scene_object;
	GLuint texture_id = 0;
	//UISceneHierarchyDrawer ui_hiearchy_drawer;
	//UIObjectPropertyDrawer ui_property_drawer;
public:
	void draw_object_hierarchy();
	void draw_object_properties();
	void draw_selected_icon(glm::mat4 a_view, glm::mat4 a_proj);
	void set_scene(std::weak_ptr<Scene> a_scene);
	void set_draw_texture(GLuint a_texture_id);
};

