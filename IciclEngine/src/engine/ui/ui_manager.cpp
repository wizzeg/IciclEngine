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
		if (ImGui::Button("New Scene Object"))
			scene_ptr->new_scene_object("scene object (" + std::to_string(scene_ptr->get_next_index()) + ")", true);

		auto root_scene_objects = scene_ptr->get_root_scene_objects();
		for (size_t i = 0; i < root_scene_objects.size(); i++)
		{
			if (root_scene_objects[i]->is_runtime() && !scene_ptr->get_registry().valid(root_scene_objects[i]->get_entity()))
			{
				scene_ptr->destroy_scene_object(root_scene_objects[i]);
			}
			else
			{
				ui_hiearchy_drawer.draw_hierarchy_node(root_scene_objects[i]);
			}
			
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
			if (ImGui::Button("Add Component"))
			{
				if (auto shared_scene = scene.lock())
				{

				}
			}
			ImGui::SameLine(0, 30);
			if (ImGui::Button("Delete Scene Object"))
			{
				if (auto shared_scene = scene.lock())
				{
					shared_scene->destroy_scene_object(selected);
					ui_hiearchy_drawer.selected_scene_object.reset();
					ImGui::End();
					return;
				}
			}
			else
			{
				ui_property_drawer.draw_object_properties(selected);
			}
			//if (auto shared_scene = scene.lock()) // This case is now fine. no problems
			//{
			//	shared_scene->destroy_entity(selected->get_entity());
			//}

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