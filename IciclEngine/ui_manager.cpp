#include "ui_manager.h"

#include "scene.h"
#include "scene_object.h"
#include "macros.h"

#include <imgui-docking/imgui.h>

void UIManager::draw_object_hierarchy()
{
	ImGui::SetNextWindowSize(ImVec2(500, 400));
	ImGui::Begin("scene objects");
	if (auto scene_ptr = scene.lock())
	{
		auto root_scene_objects = scene_ptr->get_root_scene_objects();
		for (size_t i = 0; i < root_scene_objects.size(); i++)
		{
			ui_hiearchy_drawer.draw_hierarchy_node(root_scene_objects[i]);
		}
	}
	ImGui::End();
}

void UIManager::draw_object_properties()
{
	ImGui::SetNextWindowSize(ImVec2(500, 400));
	ImGui::Begin("scene object components");
	if (auto scene_object = ui_hiearchy_drawer.selected_scene_object.lock())
	{
		ui_property_drawer.draw_object_properties(scene_object);
	}
	ImGui::End();
}

void UIManager::set_scene(std::weak_ptr<Scene> a_scene)
{
	scene = a_scene;
}