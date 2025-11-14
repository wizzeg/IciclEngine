#pragma once
#include <string>
#include "components.h"
#include <entt/entt.hpp>
#include <imgui-docking/imgui.h>
#include <glm/glm.hpp>

#include "macros.h"

struct ComponentData
{
	virtual const std::string get_name() const = 0;
	~ComponentData() {};

	// implementation in .cpp
	virtual bool draw_imgui(entt::handle a_handle) = 0;
	virtual void to_runtime(entt::handle a_handle) = 0;
};

struct NameComponentData : ComponentData
{
	NameComponent name_component;
	const std::string get_name() const override { return "name component"; }
	NameComponentData(NameComponent a_name) { name_component = a_name; /*PRINTLN("name_component.name: {}", name_component.name);*/ };
	NameComponentData() {};

	// implementation in .cpp
	bool draw_imgui(entt::handle a_handle) override;
	void to_runtime(entt::handle a_handle) override;
};

struct WorldPositionComponentData : ComponentData
{
	WorldPositionComponent worldpos_component;
	const std::string get_name() const override { return "world position component"; }
	WorldPositionComponentData(const WorldPositionComponent a_world_pos) { worldpos_component = a_world_pos; };
	WorldPositionComponentData() : worldpos_component({glm::vec3(0,0 ,0)}) {};

	// implementation in .cpp
	bool draw_imgui(entt::handle a_handle) override;
	void to_runtime(entt::handle a_handle) override;
};