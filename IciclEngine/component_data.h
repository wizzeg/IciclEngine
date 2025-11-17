#pragma once
#include <string>
#include "components.h"
#include <entt/entt.hpp>
#include <imgui-docking/imgui.h>
#include <glm/glm.hpp>

#include "macros.h"

struct FieldInfo
{
	std::string name;
	std::type_index type;
	void* value_ptr;
};

struct ComponentData
{
	virtual const std::string get_name() const = 0;
	~ComponentData() {};

	// implementation in .cpp
	virtual bool draw_imgui(entt::handle a_handle, bool runtime) = 0;
	virtual void to_runtime(entt::handle a_handle) = 0;
	virtual std::vector<FieldInfo> get_field_info() = 0;
};

struct NameComponentData : ComponentData
{
	NameComponent name_component;
	const std::string get_name() const override { return "Entity name component"; }
	NameComponentData(NameComponent a_name) : name_component(a_name) { };
	NameComponentData() {};

	// implementation in .cpp
	bool draw_imgui(entt::handle a_handle, bool runtime) override;
	void to_runtime(entt::handle a_handle) override;
	std::vector<FieldInfo> get_field_info() override;
};

struct WorldPositionComponentData : ComponentData
{
	WorldPositionComponent worldpos_component;
	const std::string get_name() const override { return "world position component"; }
	WorldPositionComponentData(const WorldPositionComponent a_world_pos) { worldpos_component = a_world_pos; };
	WorldPositionComponentData() : worldpos_component({glm::vec3(0,0 ,0)}) {};

	// implementation in .cpp
	bool draw_imgui(entt::handle a_handle, bool runtime) override;
	void to_runtime(entt::handle a_handle) override;
	std::vector<FieldInfo> get_field_info() override;
};

struct RenderableComponentData : ComponentData
{
	RenderableComponent renderable;
	const std::string get_name() const override { return "renderable component"; }
	RenderableComponentData(const RenderableComponent a_renderable) { renderable = a_renderable; };
	RenderableComponentData() : renderable({0, 0}) {};

	// implementation in .cpp
	bool draw_imgui(entt::handle a_handle, bool runtime) override;
	void to_runtime(entt::handle a_handle) override;
	std::vector<FieldInfo> get_field_info() override;
};