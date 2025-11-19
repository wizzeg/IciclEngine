#include "ui_object_property_drawer.h"
#include "scene_object.h"
#include <glm/glm.hpp>
#include <string>
#include "component_data.h"

void UIObjectPropertyDrawer::draw_object_properties(std::weak_ptr<SceneObject> a_scene_object)
{
	if (auto scene_object = a_scene_object.lock())
	{
		ImGui::BeginGroup();
		//scene_object->draw_components();
		const auto& component_datas = scene_object->get_component_datas();
		for (size_t i = 0; i < component_datas.size(); i++)
		{
			//ImGui::SeparatorText(component_datas[i]->get_name().c_str());
			//component_datas[i]->draw_imgui(scene_object->get_entity_handle(), scene_object->is_runtime());
			std::vector<FieldInfo> field_info = component_datas[i]->get_field_info();
			float field_size = 0.33f;
			for (size_t i = 0; i < field_info.size(); i++)
			{
				field_size += field_info[i].imgui_size;
			}
			ImVec2 child_size = ImVec2(0, ImGui::GetFrameHeightWithSpacing() * field_size);
			ImGui::BeginChild(component_datas[i]->get_name().c_str(), child_size , true, ImGuiChildFlags_AutoResizeY);
			draw_component_fields(field_info);
			ImGui::EndChild();
		}
		ImGui::EndGroup();
	}
}

void UIObjectPropertyDrawer::draw_component_fields(std::vector<FieldInfo> a_field_info)
{
	for (const auto& field : a_field_info)
	{
		if (field.type == typeid(float))
		{

			ImGui::DragFloat(field.name.c_str(), static_cast<float*>(field.value_ptr));
		}
		else if (field.type == typeid(glm::vec3))
		{
			ImGui::Text(field.name.c_str());
			//ImGui::SameLine();
			ImGui::DragFloat3("", &reinterpret_cast<glm::vec3*>(field.value_ptr)->x);
		}
		else if (field.type == typeid(std::string))
		{
			std::string* string_ptr = reinterpret_cast<std::string*>(field.value_ptr);
			std::string final_string = field.name + *string_ptr;
			ImGui::Text(final_string.c_str());
		}
		else if (field.type == typeid(uint32_t))
		{
			uint32_t* value = static_cast<uint32_t*>(field.value_ptr);
			std::string final_string = field.name + std::to_string(*value);
			ImGui::Text(final_string.c_str());
		}
	}
}
