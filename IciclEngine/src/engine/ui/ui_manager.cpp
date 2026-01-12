#include <engine/ui/ui_manager.h>

#include <engine/editor/scene.h>
#include <engine/editor/scene_object.h>
#include <engine/utilities/macros.h>

#include <imgui-docking/imgui.h>
#include <engine/editor/component_registry.h>
#include <engine/editor/component_factory.h>


void UIManager::draw_object_hierarchy()
{
	if (shoud_draw_object_hierarchy)
	{
		if (auto scene_ptr = scene.lock())
		{
			//ImGui::SetNextWindowSize(ImVec2(500, 400));
			ImGui::Begin("scene objects", &shoud_draw_object_hierarchy);
			if (ImGui::Button("New Scene Object"))
				scene_ptr->new_scene_object("scene object (" + std::to_string(scene_ptr->get_next_index()) + ")", true);
			ImGui::SameLine();
			static bool open_save_popup = false;
			static bool open_load_popup = false;
			static char path[260] = "";
			if (ImGui::Button("Save Scene"))
			{
				open_save_popup = true;
				ImGui::OpenPopup("Save Scene As ... ");
			}
			ImGui::SameLine();
			if (ImGui::Button("Load Scene"))
			{
				open_load_popup = true;
				ImGui::OpenPopup("Load Scene at ...");
			}

			if (open_load_popup && open_save_popup)
			{
				open_load_popup = false;
				open_save_popup = false;
			}

			if (ImGui::BeginPopupModal("Save Scene As ... ", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("path: ");
				ImGui::SameLine();
				ImGui::InputText("##Path", path, IM_ARRAYSIZE(path));
				std::string full_path = "./assets/" + std::string(path) + ".scn";
				ImGui::Text(full_path.c_str());
				if (ImGui::Button("Save", ImVec2(120, 0)))
				{
					open_save_popup = false;
					scene_ptr->save(full_path);
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(120, 0)))
				{
					open_save_popup = false;
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			if (ImGui::BeginPopupModal("Load Scene at ...", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("path: ");
				ImGui::SameLine();
				ImGui::InputText("##Path", path, IM_ARRAYSIZE(path));
				std::string full_path = "./assets/" + std::string(path) + ".scn";
				ImGui::Text(full_path.c_str());

				if (ImGui::Button("Load", ImVec2(120, 0)))
				{
					open_load_popup = false;
					scene_ptr->load(full_path, true);
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(120, 0)))
				{
					open_load_popup = false;
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
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
	else
	{
		ImGui::Begin("ui manager");
		ImGui::Checkbox("draw ui content: ", &shoud_draw_object_hierarchy);
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

void UIManager::draw_selected_icon(glm::mat4 a_view, glm::mat4 a_proj)
{
	//if (auto selected = UISceneHierarchyDrawer::selected_scene_object.lock())
	//{
	//	if (auto world_pos = selected->try_get_component<TransformDynamicComponent>())
	//	{
	//		glm::vec4 clip_pos = a_proj * a_view * glm::vec4(world_pos->get_component().position, 1.0f);
	//		glm::vec2 screen_pos((clip_pos.x / clip_pos.w + 1.0f) * 0.5f * 10.f,
	//			(1.0f - clip_pos.y / clip_pos.w) * 0.5f * 10.f);

	//		ImGui::Begin("ViewportOverlay", nullptr,
	//			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
	//			ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);
	//		ImGui::SetCursorScreenPos(ImVec2(screen_pos.x - 5, screen_pos.y - 5));  // Center 10x10 icon
	//		ImGui::Image((ImTextureID)1, ImVec2(10.f, 10.f));
	//		ImGui::End();
	//	}

	//}
}

void UIManager::set_scene(std::weak_ptr<Scene> a_scene)
{
	scene = a_scene;
}

void UIManager::set_draw_texture(GLuint a_texture_id)
{
	texture_id = a_texture_id;
}
