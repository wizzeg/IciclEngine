#include <engine/ui/ui_manager.h>

#include <engine/editor/scene.h>
#include <engine/editor/scene_object.h>
#include <engine/utilities/macros.h>

#include <imgui-docking/imgui.h>
#include <engine/editor/component_registry.h>
#include <engine/editor/component_factory.h>

void UIManager::draw_object_hierarchy()
{

	if (auto scene_ptr = scene.lock())
	{
		//ImGui::SetNextWindowSize(ImVec2(500, 400));
		ImGui::Begin("scene objects");
		if (ImGui::Button("New Scene Object"))
			scene_ptr->new_scene_object("scene object (" + std::to_string(scene_ptr->get_next_index()) + ")", true);
		if (ImGui::Button("Save Scene"))
		{

		}

		auto root_scene_objects = scene_ptr->get_root_scene_objects();
		for (size_t i = 0; i < root_scene_objects.size(); i++)
		{
			if (root_scene_objects[i]->is_runtime() && !scene_ptr->get_registry().valid(root_scene_objects[i]->get_entity()))
			{
				scene_ptr->destroy_scene_object(root_scene_objects[i]);
			}
			else
			{
				draw_hierarchy_node(root_scene_objects[i]);
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
	//if (auto scene_object = ui_hiearchy_drawer.prev_selected_scene_object.lock())
	//{
		//ImGui::SetNextWindowSize(ImVec2(500, 400));
	should_draw_object_properties = !UISceneHierarchyDrawer::selected_scene_object.expired();
	if (should_draw_object_properties)
	{
		ImGui::Begin("component properties", &should_draw_object_properties);
		if (auto selected = UISceneHierarchyDrawer::selected_scene_object.lock())
		{
			if (ImGui::Button("Add Component"))
			{
				ImGui::OpenPopup("Add Component");
			}

			if (ImGui::BeginPopup("Add Component"))
			{
				if (auto shared_scene = scene.lock())
				{
					const auto& components = ComponentRegistry::instance().get_all_component_names();
					for (const auto& component_name : components)
					{
						if (ImGui::MenuItem(component_name.c_str()))
						{
							ComponentFactory::instance().create_component(component_name, selected.get());
						}
					}
				}
				ImGui::EndPopup();
			}


			ImGui::SameLine(0, 30);
			if (ImGui::Button("Delete Scene Object"))
			{
				if (auto shared_scene = scene.lock())
				{
					shared_scene->destroy_scene_object(selected);
					UISceneHierarchyDrawer::selected_scene_object.reset();
					prev_selected_scene_object.reset();
					ImGui::End();
					return;
				}
			}
			else
			{
				UIObjectPropertyDrawer::draw_object_properties(selected);
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
	if (!should_draw_object_properties) UISceneHierarchyDrawer::selected_scene_object.reset();

	//}
	//ui_hiearchy_drawer.prev_selected_scene_object.reset();

}

void UIManager::set_scene(std::weak_ptr<Scene> a_scene)
{
	scene = a_scene;
}