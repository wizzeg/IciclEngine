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
	float imgui_size = 1;
	bool is_ref = false; // if ref, we can then know that some field there wants a ref... But I could also store entt::entity and right under uint32_t, because then I know
	
};

// problem is references to other entities... this needs a custom conversion

template <typename T>
class ComponentDataaad
{
private:
	T component;
	entt::handle entity_handle;
	bool runtime = false;
	virtual std::vector<FieldInfo> get_field_info(T& a_component)
	{
		return { };
	}
public:
	ComponentDataaad(T a_component) : component(a_component) {};

	void set_new_component_value(T a_component) { component = a_component; }

	const char* get_name() const { return typeid(T).name(); }
	virtual void to_runtime(entt::handle a_handle)
	{
		if (a_handle != entt::null)
		{
			entity_handle = a_handle;
			runtime = true;
			entt::registry* registry = a_handle.registry();
			entt::entity entity = a_handle.entity();
			registry->emplace<T>(entity, component);
		}
	}

	std::vector<FieldInfo> get_field_info()
	{
		if (runtime)
		{
			if (entity_handle != entt::null)
			{
				entt::entity entity = entity_handle.entity();
				entt::registry* registry = entity_handle.registry();

				if (T* entity_component = registry->try_get<T>(entity))
				{
					return get_field_info(*entity_component);
				}
			}
		}
		else
		{
			return get_field_info(component);
		}
	}
};

class NameComponentDataaad : ComponentDataaad<NameComponent>
{
	std::vector<FieldInfo> get_field_info(NameComponent& a_component) override
	{
		return
		{
			{"entity name: ", typeid(std::string), &a_component.name }
		};
	}
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