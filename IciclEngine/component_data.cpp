#pragma once
#include "component_data.h"
#include "scene.h"



// idea make ComopnentData into template instead, always call T component, 
// then it can run typeid check and match with the staic unordered map drawer to see if user has put in a drawer.


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
};


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
				if (ImGui::DragFloat3("World Position: ", &component->position.x, 0.1f)) {
				}
				has_component = true;
			}
		}
	}
	else
	{
		if (ImGui::DragFloat3("World Position: ", &worldpos_component.position.x, 0.1f))
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
};

bool RenderableComponentData::draw_imgui(entt::handle a_handle, bool runtime)
{
	bool has_component = false;
	if (a_handle != entt::null)
	{
		auto entity = a_handle.entity();
		auto registry = a_handle.registry();

		if (auto component = registry->try_get<RenderableComponent>(entity))
		{
			ImGui::Text("mesh: " ); //+ component->meshID + ", MaterialID: " + component->mateiralID
			has_component = true;
		}
	}
	else
	{
		ImGui::Text("mesh: "); //+ component->meshID + ", MaterialID: " + component->mateiralID
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

// here make a drawer...