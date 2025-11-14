#pragma once
#include "component_data.h"
#include "scene.h"
#include "entity.h"

void ComponentData::to_runtime(entt::handle a_handle) {};


void NameComponentData::draw_imgui()
{
	ImGui::Text((("component name: " + name_component.name).c_str()));
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


void WorldPositionComponentData::draw_imgui()
{
	auto x = std::to_string(worldpos_component.position.x);
	auto y = std::to_string(worldpos_component.position.y);
	auto z = std::to_string(worldpos_component.position.z);
	ImGui::Text((("world position: " + x + " " + y + " " + z).c_str()));
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
