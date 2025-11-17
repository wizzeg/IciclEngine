#include "ui_object_property_drawer.h"
#include "scene_object.h"

void UIObjectPropertyDrawer::draw_object_properties(std::weak_ptr<SceneObject> a_scene_object)
{
	if (auto scene_object = a_scene_object.lock())
	{
		//scene_object->draw_components();
		const auto& component_datas = scene_object->get_component_datas();
		for (size_t i = 0; i < component_datas.size(); i++)
		{
			component_datas[i]->draw_imgui(scene_object->get_entity_handle(), scene_object->is_runtime());
		}
	}
}
