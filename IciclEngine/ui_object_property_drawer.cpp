#include "ui_object_property_drawer.h"
#include "scene_object.h"

void UIObjectPropertyDrawer::draw_object_properties(std::weak_ptr<SceneObject> a_scene_object)
{
	if (auto scene_object = a_scene_object.lock())
	{
		scene_object->draw_components();
	}
}
