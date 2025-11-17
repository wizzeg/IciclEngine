#pragma once
#include <memory>

class SceneObject;
struct UIObjectPropertyDrawer
{
	void draw_object_properties(std::weak_ptr<SceneObject> a_scene_object);

	//template<typename T, typename... Args>
	//static void draw_component_data(Args&&... args)
	//{

	//}
};

