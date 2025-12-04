#include <engine/ui/ui_object_property_drawer.h>
#include <engine/editor/scene_object.h>
#include <glm/glm.hpp>
#include <string>
#include <engine/game/component_data.h>
#include <random>

void UIObjectPropertyDrawer::draw_object_properties(std::shared_ptr<SceneObject>& a_scene_object) /// need to cache the fields if they're selected
{
	
	//if (std::shared_ptr<SceneObject> scene_object = a_scene_object.lock())
	//{
		std::srand(std::time(0));
		ImGui::BeginGroup();
		bool drawn_anything = false;
		const auto& component_datas = a_scene_object->get_component_datas();
		for (size_t i = 0; i < component_datas.size(); i++)
		{
			std::vector<FieldInfo> field_info = component_datas[i]->get_field_info();
			float field_size = 1.33f;
			//std::string id_salt = " ";
			for (size_t i = 0; i < field_info.size(); i++)
			{
				//id_salt += field_info[i].name;
				field_size += field_info[i].imgui_size;
				drawn_anything = true;
			}
			ImVec2 child_size = ImVec2(0, ImGui::GetFrameHeightWithSpacing() * field_size);
			ImGui::BeginChild((std::to_string((uint32_t)a_scene_object->get_entity() + std::rand())).c_str(), child_size, true, ImGuiChildFlags_AutoResizeY); // problem, it does not display name, it's an ID.. Need to make an ID system if I want to do this
			ImGui::SeparatorText(component_datas[i]->get_name());
			draw_component_fields(field_info);
			ImGui::EndChild();
		}
		if (!drawn_anything)
		{
			ImGui::Text("No properties found");
			PRINTLN("Nothing drawn");
		}
		ImGui::EndGroup();
	//}
}

void UIObjectPropertyDrawer::draw_component_fields(std::vector<FieldInfo>& a_field_info)
{
	int i = 0;
	for (const auto& field : a_field_info)
	{
		
		if (field.type == typeid(float))
		{
			ImGui::DragFloat(field.name.c_str(), static_cast<float*>(field.value_ptr));
		}
		else if (field.type == typeid(bool))
		{
			ImGui::Checkbox(field.name.c_str(), static_cast<bool*>(field.value_ptr));
		}
		else if (field.type == typeid(glm::vec3))
		{
			ImGui::Text(field.name.c_str());
			//ImGui::SameLine();
			std::string salt = "##" + field.name + std::to_string(i++);
			ImGui::DragFloat3(salt.c_str(), &reinterpret_cast<glm::vec3*>(field.value_ptr)->x);
		}
		else if (field.type == typeid(std::string))
		{
			std::string* string_ptr = reinterpret_cast<std::string*>(field.value_ptr);
			std::string final_string = field.name + *string_ptr;
			char buffer[128];
			ImGui::Text(field.name.c_str());
			ImGui::SameLine();
			strncpy_s(buffer, string_ptr->c_str(), 128);
			if (ImGui::InputText(" ", buffer, 128))
			{
				*string_ptr = buffer;
			}
		}
		else if (field.type == typeid(uint32_t))
		{
			uint32_t* value = static_cast<uint32_t*>(field.value_ptr);
			std::string final_string = field.name + std::to_string(*value);
			int temp = static_cast<int>(*value);
			if (ImGui::InputInt(final_string.c_str(), &temp, 1, 100, 0))
			{
				*value = ((uint32_t)temp);
			}
		}
		else if (field.type == typeid(uint16_t))
		{
			uint16_t* value = static_cast<uint16_t*>(field.value_ptr);
			std::string final_string = field.name + std::to_string(*value);
			int temp = static_cast<int>(*value);
			if (ImGui::InputInt(final_string.c_str(), &temp, 1, 100, 0))
			{
				*value = ((uint16_t)temp);
			}
		}
		else if (field.type == typeid(uint64_t))
		{
			uint64_t* value = static_cast<uint64_t*>(field.value_ptr);
			std::string final_string = field.name + std::to_string(*value);
			int temp = static_cast<int>(*value);
			if (ImGui::InputInt(final_string.c_str(), &temp, 1, 100, 0))
			{
				*value = ((uint64_t)temp);
			}
		}
		else if (field.type == typeid(hashed_string_64))
		{
			hashed_string_64* value = reinterpret_cast<hashed_string_64*>(field.value_ptr);

			std::string final_string = field.name + value->string;
			char buffer[128];
			ImGui::Text(field.name.c_str());
			ImGui::SameLine();
			strncpy_s(buffer, value->string.c_str(), 128);
			if (ImGui::InputText(" ", buffer, 128))
			{
				*value = hashed_string_64(buffer);
			}

			std::string text = "path hash: " + std::to_string(value->hash)/*+ " - path: " + value.data()*/;
			ImGui::Text(text.c_str());
		}
		else
		{
			ImGui::Text("empty field");
		}

		if (field.same_line)
		{
			ImGui::SameLine();
		}
	}
}

void UIObjectPropertyDrawer::draw_editable_field(FieldInfo& a_field_info)
{
	if (a_field_info.edit_mode == EEditMode::Editable)
	{
		if (a_field_info.type == typeid(float))
		{
			ImGui::DragFloat(a_field_info.name.c_str(), static_cast<float*>(a_field_info.value_ptr));
		}
		else if (a_field_info.type == typeid(glm::vec3))
		{
			ImGui::Text(a_field_info.name.c_str());
			//ImGui::SameLine();
			ImGui::DragFloat3("", &reinterpret_cast<glm::vec3*>(a_field_info.value_ptr)->x);
		}
		else if (a_field_info.type == typeid(std::string))
		{
			std::string* string_ptr = reinterpret_cast<std::string*>(a_field_info.value_ptr);
			std::string final_string = a_field_info.name + *string_ptr;
			char* buffer[128] = {0};
			if (ImGui::InputText(a_field_info.name.c_str(), buffer[0], sizeof(buffer)))
			{
				*string_ptr = std::string(buffer[0]);
			}
		}
		else if (a_field_info.type == typeid(uint32_t))
		{
			uint32_t* value = static_cast<uint32_t*>(a_field_info.value_ptr);
			std::string final_string = a_field_info.name + std::to_string(*value);
			int temp = static_cast<int>(*value);
			if (ImGui::InputInt(final_string.c_str(), &temp, 1, 100, 0))
			{
				*value = ((uint32_t)temp);
			}
		}
		else if (a_field_info.type == typeid(hashed_string_64))
		{
			hashed_string_64 value = *reinterpret_cast<hashed_string_64*>(a_field_info.value_ptr);
			std::string text = "path hash: " + std::to_string(value.hash)/*+ " - path: " + value.data()*/;
			ImGui::Text(text.c_str());
		}
		else
		{
			ImGui::Text("empty field");
		}
	}
}
