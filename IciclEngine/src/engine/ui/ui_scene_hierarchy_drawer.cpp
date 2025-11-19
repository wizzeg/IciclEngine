#include "ui_scene_hierarchy_drawer.h"
#include <imgui-docking/imgui.h>
#include "macros.h"
#include "scene_object.h"

void UISceneHierarchyDrawer::draw_hierarchy_node(std::weak_ptr<SceneObject> a_scene_object)
{
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_OpenOnArrow;
	if (auto scene_object = a_scene_object.lock())
	{
		auto children = scene_object->get_children();


		if (!(children.size() > 0))
		{
			flags |= ImGuiTreeNodeFlags_Bullet;
		}
		auto node_name = scene_object->get_name();
		bool is_open = ImGui::TreeNodeEx(node_name.c_str(), flags);

		if (ImGui::IsItemClicked())
		{
			selected_scene_object = a_scene_object;
		}

		if (is_open) {
			for (size_t i = 0; i < children.size(); i++)
			{
				if (auto child = children[i].lock())
				{
					draw_hierarchy_node(child);
				}
			}
		}
		if (is_open)
		{
			ImGui::TreePop();
		}
	}
}
