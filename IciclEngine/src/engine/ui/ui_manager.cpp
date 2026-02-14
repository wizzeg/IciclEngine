#include <engine/ui/ui_manager.h>

#include <engine/editor/scene.h>
#include <engine/editor/scene_object.h>
#include <engine/utilities/macros.h>

#include <imgui-docking/imgui.h>
#include <engine/editor/component_registry.h>
#include <engine/editor/component_factory.h>
#include <engine/editor/systems_registry.h>

#include <engine/core/game_thread.h>



void UIManager::draw_menubar()
{
	menu_bar.draw_menu_bar();
}

void UIManager::draw_systems()
{
	ImGui::Begin("Systems");

	//if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
	if (ImGui::Button("Add System"))
	{
		ImGui::OpenPopup("Add System");
	}
	ImGui::SameLine();
	if (ImGui::Button("Order Systems"))
	{
		if (auto scn = scene.lock())
		{
			scn->reorder_systems();
		}
		
	}

	// Popup
	if (ImGui::BeginPopup("Add System")) {
		ImGui::Text("System Order: ");
		ImGui::SameLine();
		ImGui::DragInt("##order", &prev_order, 1.f, 0, UINT32_MAX);
		if (auto shared_scene = scene.lock())
		{
			const auto& systems = SystemsRegistry::instance().get_all_system_names();
			for (const auto& system : systems)
			{
				if (ImGui::MenuItem(system.c_str()))
				{
					auto new_system = SystemsFactory::instance().create_system(system);
					new_system->set_name(system);
					new_system->set_order((uint32_t)prev_order);
					shared_scene->add_system(new_system);
				}
			}
		}
		ImGui::EndPopup();
	}

	if (auto scn = scene.lock())
	{
		auto& systems = scn->get_systems();
		size_t id = 0;
		std::shared_ptr<SystemBase> selected;
		selected.reset();
		bool remove = false;
		for (auto& system : systems)
		{
			ImGui::PushID(id++);
			ImGui::Text((system->get_name() + " at order: ").c_str());
			int order = system->get_order();
			ImGui::SameLine();
			ImGui::SetNextItemWidth(100.0f);
			if (ImGui::DragInt("##order", &order, 1.f, 0, UINT32_MAX))
			{
				system->set_order(order);
			}
			ImGui::SameLine();
			ImGui::Text("enabled: ");
			ImGui::SameLine();
			if (bool enabled = system->get_enabled(); ImGui::Checkbox("##enabled", &enabled))
				system->set_enabled(enabled);

			ImGui::SameLine();
			ImGui::Text(", physics frame: ");
			ImGui::SameLine();
			if (bool physics_only = system->get_physics_frames_only(); ImGui::Checkbox("##physics", &physics_only))
				system->set_only_on_physics(physics_only);

			ImGui::SameLine();
			if (ImGui::Button("remove system"))
			{
				ImGui::PopID();
				scn->remove_system(system);
				scn->reorder_systems();
				break;
			}
			ImGui::PopID();
		}
	}



	ImGui::End();
}

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

			
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_OBJECT"))
				{
					// Get the dragged object
					std::shared_ptr<SceneObject> dragged_obj = *(std::shared_ptr<SceneObject>*)payload->Data;

					// Make sure we're not dropping onto itself
					if (dragged_obj.get())
					{
						auto scene = dragged_obj->get_scene();
						if (auto scn = scene.lock())
						{
							scn->orphan_scene_object(dragged_obj);
						}
					}
				}
				ImGui::EndDragDropTarget();
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
					draw_hierarchy_node(root_scene_objects[i], i);
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

	if (UISceneHierarchyDrawer::open_load_popup)
	{
		ImGui::OpenPopup("Load Prefab at ...");
	}
	if (UISceneHierarchyDrawer::open_save_popup)
	{
		ImGui::OpenPopup("Save Prefab As ...");
	}

	if (ImGui::BeginPopupModal("Load Prefab at ...", &(UISceneHierarchyDrawer::open_load_popup), ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (auto scene_object = selected_scene_object.lock())
		{
			auto scene = scene_object->get_scene();
			ImGui::Text("path: ");
			ImGui::SameLine();
			ImGui::InputText("##Path", prefab_path, IM_ARRAYSIZE(prefab_path));
			std::string full_path = "./assets/" + std::string(prefab_path) + ".prfb";
			ImGui::Text(full_path.c_str());

			if (ImGui::Button("Load", ImVec2(120, 0)))
			{
				UISceneHierarchyDrawer::open_load_popup = false;
				if (auto scn = scene.lock())
					scn->load_prefab(full_path);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				UISceneHierarchyDrawer::open_load_popup = false;
				ImGui::CloseCurrentPopup();
			}
		}
		
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Save Prefab As ...", &(UISceneHierarchyDrawer::open_save_popup), ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (auto scene_object = selected_scene_object.lock())
		{
			auto scene = scene_object->get_scene();
			ImGui::Text("path: ");
			ImGui::SameLine();
			ImGui::InputText("##Path", prefab_path, IM_ARRAYSIZE(prefab_path));
			std::string full_path = "./assets/" + std::string(prefab_path) + ".prfb";
			ImGui::Text(full_path.c_str());
			if (ImGui::Button("Save", ImVec2(120, 0)))
			{
				UISceneHierarchyDrawer::open_save_popup = false;
				if (auto scn = scene.lock())
					scn->save_prefab(full_path, scene_object);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				UISceneHierarchyDrawer::open_save_popup = false;
				ImGui::CloseCurrentPopup();
			}

		}
		ImGui::EndPopup();
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
	// POP ID missing when removing component.

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
	menu_bar.set_scene(a_scene);
}

void UIManager::set_draw_texture(GLuint a_texture_id)
{
	texture_id = a_texture_id;
}

void UIManager::render_play_stop(EngineContext* a_engine_context)
{
	ImGui::PushID("playstop");
	ImGui::Begin("playstop");
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 3));

	// Play button - UNIQUE ID "Play##MenuBar"
	if (ImGui::Button(playback.isPlaying && !playback.isPaused ? "Pause##PlayBtn" : "Play##PlayBtn", ImVec2(45, 18))) {
		if (playback.isPlaying && !playback.isPaused) {
			playback.isPaused = true;
			a_engine_context->game_paused = true;
		}
		else {
			playback.isPlaying = true;
			playback.isPaused = false;
			a_engine_context->game_paused = false;
		}
		//std::lock_guard<std::mutex> guard(a_engine_context->mutex);
		//a_scene->start_runtime(); // deal with making a runtime copy later -------- runtime thing works at least, entities are created
		a_engine_context->start_game_thread(true);

	}

	ImGui::SameLine();

	// Stop button - UNIQUE ID "Stop##MenuBar"  
	if (ImGui::Button("Stop##StopBtn", ImVec2(45, 18))) {
		playback.isPlaying = false;
		playback.isPaused = false;

		//set playing to false.
		a_engine_context->start_game_thread(false);
	}

	ImGui::PopStyleVar(2);
	ImGui::End();
	ImGui::PopID();
}