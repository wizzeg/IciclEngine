#include <engine/ui/ui_manager.h>

#include <engine/editor/scene.h>
#include <engine/editor/scene_object.h>
#include <engine/utilities/macros.h>

#include <imgui-docking/imgui.h>

void UIManager::draw_object_hierarchy()
{

	if (auto scene_ptr = scene.lock())
	{
		//ImGui::SetNextWindowSize(ImVec2(500, 400));
		ImGui::Begin("scene objects");
		auto root_scene_objects = scene_ptr->get_root_scene_objects();
		for (size_t i = 0; i < root_scene_objects.size(); i++)
		{
			ui_hiearchy_drawer.draw_hierarchy_node(root_scene_objects[i]);
		}
		ImGui::End();
	}
}

void UIManager::draw_object_properties()
{
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// BUG, if I double click on this window it breaks (but now I can't reproduce it again...)
	// 
	//if (auto scene_object = ui_hiearchy_drawer.selected_scene_object.lock())
	//{
		//ImGui::SetNextWindowSize(ImVec2(500, 400));
	
	if (!ui_hiearchy_drawer.selected_scene_object.expired())
	{
		open = true;
	}
	if (open)
	{
		ImGui::Begin("component properties", &open);
		if (auto selected = ui_hiearchy_drawer.selected_scene_object.lock())
		{
			ui_property_drawer.draw_object_properties(selected);
		}
		else
		{
			ImGui::Text("No scene object selected");
		}
		ImGui::End();
	}
	if (!open) ui_hiearchy_drawer.selected_scene_object.reset();

	//}
	//ui_hiearchy_drawer.selected_scene_object.reset();

}

void UIManager::set_scene(std::weak_ptr<Scene> a_scene)
{
	scene = a_scene;
}