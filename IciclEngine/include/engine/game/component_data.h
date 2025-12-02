#pragma once
#include <string>
//#include "components.h"
#include <engine/game/components.h>
#include <engine/utilities/entt_modified.h>
//#include <entt/entt.hpp>
#include <imgui-docking/imgui.h>
#include <glm/glm.hpp>

//#include "macros.h"
#include <engine/utilities/macros.h>
enum EEditMode
{
	Editable,
	Uneditable,
};

struct FieldInfo
{
	EEditMode edit_mode;
	std::string name;
	std::type_index type;
	void* value_ptr;
	float imgui_size = 1;
};

// problem is references to other entities... 
// this needs a custom conversion -> 
// struct of EntityReference {entt::entity and uint32_t "ptr" (into uint32_t to scene_object wptr manager)}
struct ComponentDataBase
{
	ComponentDataBase() {}
	virtual const std::type_info& get_type() = 0;
	virtual void to_runtime(entt::handle a_handle) = 0;
	virtual std::vector<FieldInfo> get_field_info() = 0;
	virtual const char* get_name() const = 0;
};

template <typename TComponent>
struct ComponentData : ComponentDataBase
{
	ComponentData(TComponent a_component) : component(a_component) {};
	TComponent& get_component()
	{
		if (entity_handle.entity() != entt::null && runtime)
		{
			auto entity = entity_handle.entity();
			auto registry = entity_handle.registry();
			
			
			if (TComponent* comp = registry->try_get<TComponent>(entity))
			{
				return *comp;
			}
		}
		return component;
	} // seems to work runtime if(TComponent* component; scene_object->try_get<TComponent>(component))

	const std::type_info& get_type() override { return typeid(TComponent); }
	virtual std::vector<FieldInfo> get_field_info(TComponent& a_component)
	{
		return { };
	}

	void set_new_component_value(TComponent a_component)
	{ 
		if (entity_handle.entity() != entt::null && runtime)
		{
			auto entity = entity_handle.entity();
			auto registry = entity_handle.registry();
			if (TComponent* comp = registry->try_get<TComponent>(entity))
			{
				*comp = a_component;
			}
		}
		component = a_component;
	} // might work at runtime

	const char* get_name() const override { return typeid(TComponent).name(); }
	virtual void to_runtime(entt::handle a_handle) override
	{
		if (a_handle != entt::null)
		{
			entity_handle = a_handle;
			runtime = true;
			entt::registry* registry = a_handle.registry();
			entt::entity entity = a_handle.entity();
			registry->emplace<TComponent>(entity, component);
		}
	}

	std::vector<FieldInfo> get_field_info() override
	{
		if (runtime)
		{
			if (entity_handle != entt::null)
			{
				entt::entity entity = entity_handle.entity();
				entt::registry* registry = entity_handle.registry();

				if (TComponent* entity_component = registry->try_get<TComponent>(entity))
				{
					return get_field_info(*entity_component);
				}
				else
				{
					PRINTLN("Component data attempted to retrieve entity component that doesn't exist");
					return std::vector<FieldInfo>{};
				}
			}
			else
			{
				PRINTLN("Component data attempted to retrieve non-existing entity");
				return std::vector<FieldInfo>{};
			}
		}
		return get_field_info(component);
	}

protected:
	TComponent component;
	entt::handle entity_handle;
	bool runtime = false;

};

struct NameComponentData : ComponentData<NameComponent>
{
	NameComponentData(NameComponent a_component) : ComponentData<NameComponent>(a_component) {}
	std::vector<FieldInfo> get_field_info(NameComponent& a_component) override
	{
		return
		{
			{EEditMode::Editable, "entity name: ", typeid(std::string), &a_component.name, 1.25f }
		};
	}
};

struct WorldPositionComponentData : ComponentData<WorldPositionComponent>
{
	WorldPositionComponentData(WorldPositionComponent a_component) : ComponentData<WorldPositionComponent>(a_component){}
	std::vector<FieldInfo> get_field_info(WorldPositionComponent& a_component) override
	{
		return
		{
			{EEditMode::Editable, "world position: ", typeid(glm::vec3), &a_component.position.x, 2.f }
		};
	}
};

struct MeshLoaderComponentData : ComponentData<MeshLoaderComponent>
{
	MeshLoaderComponentData(MeshLoaderComponent a_component) : ComponentData<MeshLoaderComponent>(a_component) {}
	std::vector<FieldInfo> get_field_info(MeshLoaderComponent& a_component) override
	{
		return
		{
			{EEditMode::Editable, "mesh path: ", typeid(std::string), &a_component.path }
		};
	}
};

struct MaterialLoaderComponentData : ComponentData<MaterialLoaderComponent>
{
	MaterialLoaderComponentData(MaterialLoaderComponent a_component) : ComponentData<MaterialLoaderComponent>(a_component) {}
	std::vector<FieldInfo> get_field_info(MaterialLoaderComponent& a_component) override
	{
		return
		{
			{EEditMode::Editable, "material path: ", typeid(std::string), &a_component.path }
		};
	}
};

struct MeshComponentData : ComponentData<MeshComponent>
{
	MeshComponentData(MeshComponent a_component) : ComponentData<MeshComponent>(a_component) {}
	std::vector<FieldInfo> get_field_info(MeshComponent& a_component) override
	{
		return
		{
			{EEditMode::Editable, "mesh id: ", typeid(uint32_t), &a_component.id },
			{EEditMode::Editable, "mesh: ", typeid(hashed_string_64), &a_component.hashed_path, 2.25 }
			//{EEditMode::Editable, "mesh path: ", typeid(std::string), &a_component.hashed_path.string },
			//{EEditMode::Uneditable, "hashed path: ", typeid(uint64_t),& a_component.hashed_path.hash }
		};
	}
};

struct MaterialComponentData : ComponentData<MaterialComponent>
{
	MaterialComponentData(MaterialComponent a_component) : ComponentData<MaterialComponent>(a_component) {}
	std::vector<FieldInfo> get_field_info(MaterialComponent& a_component) override
	{
		return
		{
			{EEditMode::Editable, "material id: ", typeid(uint32_t), &a_component.id }
		};
	}
};

struct RenderableComponentData : ComponentData<RenderableComponent>
{
	RenderableComponentData(RenderableComponent a_component) : ComponentData<RenderableComponent>(a_component) {}
	std::vector<FieldInfo> get_field_info(RenderableComponent& a_component) override
	{
		return
		{
			{EEditMode::Editable, "mesh id: ", typeid(uint32_t), &a_component.mesh_id },
			{EEditMode::Editable, "material id: ", typeid(uint32_t), &a_component.mateiral_id }
		};
	}
};

struct CameraComponentData : ComponentData<CameraComponent>
{
	CameraComponentData(CameraComponent a_component) : ComponentData<CameraComponent>(a_component) {}
	std::vector<FieldInfo> get_field_info(CameraComponent& a_component) override
	{
		return
		{
			{ EEditMode::Editable, "camera active: ", typeid(bool), &a_component.render_from },
			{EEditMode::Editable, "buffer: ", typeid(hashed_string_64), &a_component.buffer_target, 2.25 }
			//{ EEditMode::Editable, "buffer target: ", typeid(std::string), &a_component.buffer_target.string },
			//{ EEditMode::Editable, "buffer hash: ", typeid(uint64_t),& a_component.buffer_target.hash }
		};
	}
};





//struct ComponentData
//{
//	virtual const std::string get_name() const = 0;
//	~ComponentData() {};
//
//	// implementation in .cpp
//	virtual bool draw_imgui(entt::handle a_handle, bool runtime) = 0;
//	virtual void to_runtime(entt::handle a_handle) = 0;
//
//	
//	virtual std::vector<FieldInfo> get_field_info() = 0;
//};
//
//struct NameComponentData : ComponentData
//{
//	NameComponent name_component;
//	const std::string get_name() const override { return "Entity name component"; }
//	NameComponentData(NameComponent a_name) : name_component(a_name) { };
//	NameComponentData() {};
//
//	// implementation in .cpp
//	bool draw_imgui(entt::handle a_handle, bool runtime) override;
//	void to_runtime(entt::handle a_handle) override;
//	std::vector<FieldInfo> get_field_info() override;
//};
//
//struct WorldPositionComponentData : ComponentData
//{
//	WorldPositionComponent worldpos_component;
//	const std::string get_name() const override { return "world position component"; }
//	WorldPositionComponentData(const WorldPositionComponent a_world_pos) { worldpos_component = a_world_pos; };
//	WorldPositionComponentData() : worldpos_component({glm::vec3(0,0 ,0)}) {};
//
//	// implementation in .cpp
//	bool draw_imgui(entt::handle a_handle, bool runtime) override;
//	void to_runtime(entt::handle a_handle) override;
//	std::vector<FieldInfo> get_field_info() override;
//};
//
//struct RenderableComponentData : ComponentData
//{
//	RenderableComponent renderable;
//	const std::string get_name() const override { return "renderable component"; }
//	RenderableComponentData(const RenderableComponent a_renderable) { renderable = a_renderable; };
//	RenderableComponentData() : renderable({0, 0}) {};
//
//	// implementation in .cpp
//	bool draw_imgui(entt::handle a_handle, bool runtime) override;
//	void to_runtime(entt::handle a_handle) override;
//	std::vector<FieldInfo> get_field_info() override;
//};