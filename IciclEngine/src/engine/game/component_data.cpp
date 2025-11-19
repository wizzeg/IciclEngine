#pragma once
#include <engine/game/component_data.h>
#include <engine/editor/scene.h>



// make component data have a default function call to get typeinfo to draw
// then user specificed gef_field_info that takes in type T, so that it can properly draw, 
// but the default function determines if it's entt component or scene objet component

bool NameComponentData::draw_imgui(entt::handle a_handle, bool runtime)
{
	bool has_component = false;
	if (runtime)
	{
		if (a_handle != entt::null)
		{
			auto entity = a_handle.entity();
			auto registry = a_handle.registry();

			if (auto component = registry->try_get<NameComponent>(entity))
			{
				ImGui::Text((("component name: " + component->name).c_str()));
				has_component = true;
			}
		}
	}
	else
	{
		ImGui::Text((("component name: " + name_component.name).c_str()));
	}
	return has_component;
}
void NameComponentData::to_runtime(entt::handle a_handle)
{
	if (a_handle != entt::null)
	{
		auto* registry = a_handle.registry();
		auto entity = a_handle.entity();

		registry->emplace<NameComponent>(entity, name_component);
	}
}
std::vector<FieldInfo> NameComponentData::get_field_info()
{
	return
	{
		{"entity name: ", typeid(std::string), &name_component.name }
	};
}



bool WorldPositionComponentData::draw_imgui(entt::handle a_handle, bool runtime) // need to draw differently if it's runtime or not.
{
	bool has_component = false;
	if (runtime)
	{
		if (a_handle != entt::null)
		{
			auto entity = a_handle.entity();
			auto registry = a_handle.registry();

			if (auto component = registry->try_get<WorldPositionComponent>(entity))
			{
				if (ImGui::DragFloat3("World Position ssdfsdf: ", &component->position.x, 0.1f)) {
				}
				has_component = true;
			}
		}
	}
	else
	{
		if (ImGui::DragFloat3("World Position asdasd: ", &worldpos_component.position.x, 0.1f))
		{

		}
	}
	return has_component;
}
void WorldPositionComponentData::to_runtime(entt::handle a_handle)
{
	if (a_handle != entt::null)
	{
		auto* registry = a_handle.registry();
		auto entity = a_handle.entity();
		registry->emplace<WorldPositionComponent>(entity, worldpos_component);
	}
}
std::vector<FieldInfo> WorldPositionComponentData::get_field_info()
{
	return
	{
		{"world position:", typeid(glm::vec3), &worldpos_component.position.x , 2.0f}
	};
}

bool RenderableComponentData::draw_imgui(entt::handle a_handle, bool runtime)
{
	bool has_component = false;
	if (a_handle != entt::null)
	{
		auto entity = a_handle.entity();
		auto registry = a_handle.registry();

		if (auto component = registry->try_get<RenderableComponent>(entity))
		{
			ImGui::Text("mesh: " ); //+ component->mesh_id + ", MaterialID: " + component->mateiral_id
			has_component = true;
		}
	}
	else
	{
		ImGui::Text("mesh: "); //+ component->mesh_id + ", MaterialID: " + component->mateiral_id
		has_component = true;
	}
	return has_component;
}
void RenderableComponentData::to_runtime(entt::handle a_handle)
{
	if (a_handle != entt::null)
	{
		auto* registry = a_handle.registry();
		auto entity = a_handle.entity();
		registry->emplace<RenderableComponent>(entity, renderable);
	}
};
std::vector<FieldInfo> RenderableComponentData::get_field_info()
{
	return
	{
		{"mesh id:", typeid(uint32_t), &renderable.mesh_id , 1.f },
		{"material id:", typeid(uint32_t), &renderable.mateiral_id, 1.f }
	};
}


// here make a drawer...