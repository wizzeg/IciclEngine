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

		if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
			selected_scene_object = a_scene_object;
			ImGui::OpenPopup("SceneObjectContextMenu");
		}
		if (ImGui::BeginPopup("SceneObjectContextMenu")) {

			if (ImGui::Selectable("Delete")) {
				// Handle delete
				auto scene = scene_object->get_scene();
				if (auto shared_scene = scene.lock())
				{
					shared_scene->destroy_scene_object(selected_scene_object);
					UISceneHierarchyDrawer::selected_scene_object.reset();
				}
			}
			if (ImGui::Selectable("Save as Prefab")) {
				// Handle properties
					open_save_popup = true;
			}
			if (ImGui::Selectable("Duplicate")) {
				// Handle duplicate
				if (auto scn = scene_object->get_scene().lock())
				{
					scn->save_prefab("./assets/temp/temp_prefab.prfb",scene_object);
					scn->load_prefab("./assets/temp/temp_prefab.prfb");
				}
			}
			ImGui::Separator();
			if (ImGui::Selectable("Add Child")) {
				// Handle properties
				
			}
			if (ImGui::Selectable("Orphan")) {
				// Handle properties
				if (auto scn = scene_object->get_scene().lock())
					scn->orphan_scene_object(scene_object);
			}
			ImGui::Separator();
			if (ImGui::Selectable("Insert prefab")) {
				// Handle properties
				open_load_popup = true;
			}
			ImGui::EndPopup();
		}


		// drag source
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			// store the scene object pointer in the payload name
			auto dragged_obj = scene_object;
			ImGui::SetDragDropPayload("SCENE_OBJECT", &dragged_obj, sizeof(std::shared_ptr<SceneObject>));
			ImGui::Text("Move: %s", node_name.c_str());
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			// get the dragged object by payload name
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_OBJECT"))
			{
				std::shared_ptr<SceneObject> dragged_obj = *(std::shared_ptr<SceneObject>*)payload->Data;

				// make sure not dropped into self
				if (dragged_obj.get() != scene_object.get())
				{
					auto scene = dragged_obj->get_scene();
					if (auto scn = scene.lock())
					{
						//scn->orphan_scene_object(dragged_obj);
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