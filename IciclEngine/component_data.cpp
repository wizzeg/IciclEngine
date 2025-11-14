#pragma once
#include "component_data.h"
#include "scene.h"


bool NameComponentData::draw_imgui(entt::handle a_handle)
{
	bool has_component = false;
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
};


bool WorldPositionComponentData::draw_imgui(entt::handle a_handle)
{
	bool has_component = false;
	if (a_handle != entt::null)
	{
		auto entity = a_handle.entity();
		auto registry = a_handle.registry();

		if (auto component = registry->try_get<WorldPositionComponent>(entity))
		{
			if (ImGui::DragFloat3("My Vector3", &component->position.x, 0.1f)) {
				PRINTLN("position changed");
			}

			//auto x = std::to_string(component->position.x);
			//auto y = std::to_string(component->position.y);
			//auto z = std::to_string(component->position.z);
			//ImGui::Text((("world position: " + x + " " + y + " " + z).c_str()));
			has_component = true;
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
};
