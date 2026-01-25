#include <engine/ui/ui_scene_hierarchy_drawer.h>
#include <imgui-docking/imgui.h>
#include <engine/utilities/macros.h>
#include <engine/editor/scene_object.h>
#include <engine/editor/scene.h>

void UISceneHierarchyDrawer::draw_hierarchy_node(std::weak_ptr<SceneObject> a_scene_object,size_t a_id)
{
	
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DrawLinesToNodes | ImGuiTreeNodeFlags_OpenOnArrow;
	if (auto scene_object = a_scene_object.lock())
	{
		ImGui::PushID(a_id);
		bool draw_orpham = false;
		auto children = scene_object->get_children();
		if (!(children.size() > 0))
		{
			flags |= ImGuiTreeNodeFlags_Bullet;
		}
		if (auto sel_obj = selected_scene_object.lock())
		{
			if (sel_obj.get() == scene_object.get())
			{
				flags |= ImGuiTreeNodeFlags_Selected;
				draw_orpham = true;
			}
		}
		auto node_name = scene_object->get_name();
		if (node_name == "")
		{
			node_name = " ";
		}
		//ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5.0f, 1.0f));
		bool is_open = ImGui::TreeNodeEx(node_name.c_str(), flags | ImGuiTreeNodeFlags_FramePadding);
		//ImGui::PopStyleVar();
		


		

		if (ImGui::IsItemClicked())
		{
			selected_scene_object = a_scene_object;
		}
		// Drag source - what you're dragging
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			// Store the scene object pointer in the payload
			// Use a raw pointer since ImGui needs the data to persist
			auto dragged_obj = scene_object;
			ImGui::SetDragDropPayload("SCENE_OBJECT", &dragged_obj, sizeof(std::shared_ptr<SceneObject>));
			ImGui::Text("Move: %s", node_name.c_str());
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_OBJECT"))
			{
				// Get the dragged object
				std::shared_ptr<SceneObject> dragged_obj = *(std::shared_ptr<SceneObject>*)payload->Data;

				// Make sure we're not dropping onto itself
				if (dragged_obj.get() != scene_object.get())
				{
					auto scene = dragged_obj->get_scene();
					if (auto scn = scene.lock())
					{
						scn->orphan_scene_object(dragged_obj);
						scn->parent_scene_object(scene_object, dragged_obj);
					}
				}
			}
			ImGui::EndDragDropTarget();
		}
		if (draw_orpham)
		{
			ImGui::SameLine(0.f, 20.f);
			if (ImGui::Button("orphan scene object", ImVec2(140.f, 20.f)))
				if (auto scn = scene_object->get_scene().lock())
					scn->orphan_scene_object(scene_object);
		}

		if (is_open)
		{
			for (size_t i = 0; i < children.size(); i++)
			{
				if (auto child = children[i].lock())
				{
					//ImGui::PushID(i);
					draw_hierarchy_node(child, (a_id+1));
					//ImGui::PopID();
				}
			}
		}
		
		if (is_open)
		{
			ImGui::TreePop();
		}
		ImGui::PopID();
	}
	
}


//void UISceneHierarchyDrawer::draw_hierarchy_node(std::weak_ptr<SceneObject> a_scene_object)
//{
//	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_OpenOnArrow;
//	if (auto scene_object = a_scene_object.lock())
//	{
//		auto children = scene_object->get_children();
//		if (!(children.size() > 0))
//			flags |= ImGuiTreeNodeFlags_Bullet;
//
//		auto node_name = scene_object->get_name();
//		if (ImGui::TreeNodeEx(node_name.c_str(), flags))
//		{
//			if (ImGui::IsItemClicked())
//				prev_selected_scene_object = a_scene_object;
//
//			for (size_t i = 0; i < children.size(); i++)
//				if (auto child = children[i].lock())
//					draw_hierarchy_node(child);
//			ImGui::TreePop();
//		}
//	}
//}