#pragma once
#include <engine/utilities/entt_modified.h>
#include <engine/editor/scene.h>
#include <engine/core/build_state.h>
#include <engine/editor/scene_object.h>
#include <memory>

struct EntityAssembly
{
	EntityAssembly(const std::string& a_name) : name(a_name) {}
	EntityAssembly() = default;
	std::string name = "0";
	std::vector<EntityAssembly> children;
	std::vector<std::function<void(entt::registry&, entt::entity)>> component_modifiers;
};

struct EntityAssembler
{
	template <typename T, typename...Args>
	EntityAssembler& with_component(Args&&...args)
	{
		entity_assembly.component_modifiers.push_back(
			[...args = std::forward<Args>(args)]
			(entt::registry& registry, entt::entity entity) mutable
			{
				registry.emplace_or_replace<T>(entity, std::forward<Args>(args)...);
			}
		);
		return *this;
	}

	template <typename T>
	EntityAssembler& with_component()
	{
		entity_assembly.component_modifiers.push_back(
			[]
			(entt::registry& registry, entt::entity entity) mutable
			{
				registry.emplace_or_replace<T>(entity);
			}
		);
		return *this;
	}

	EntityAssembler& with_child(EntityAssembly child, const std::string& name = "0");

	EntityAssembly assemble() { return std::move(entity_assembly); }
	EntityAssembly entity_assembly;
};

struct EntityCommandBuffer
{
	void create_entity(EntityAssembly entity_assembly, entt::entity parent, const std::string& name = "0");
	void create_entity(EntityAssembly entity_assembly, const std::string& name = "0");
	void set_parent(entt::entity parent, entt::entity child);
	void orphan(entt::entity entity);
	template <typename Component, typename... Args>
	void add_component(entt::entity entity, Args&&...args)
	{
		queued_add_components.push_back
		(
			[entity, ...args = std::forward<Args>(args)](entt::registry& registry)
			{
				if (registry.valid(entity))
				{
					registry.emplace<Component>(entity, std::forward<Args>(args...));
				}
			}
		);
	}
	template <typename Component>
	void add_component(entt::entity entity)
	{
		queued_add_components.push_back
		(
			[entity](entt::registry& registry)
			{
				if (registry.valid(entity))
				{
					registry.emplace<Component>(entity);
				}
			}
		);
	}
	template <typename Component>
	void remove_component(entt::entity entity)
	{
		queued_remove_components.push_back
		(
			[entity](entt::registry& registry)
			{
				if (registry.valid(entity))
				{
					registry.remove<Component>(entity);
				}
			}
		);
	}
	void destroy_entity(entt::entity entity);
	void execute_queue(entt::registry& registry);
	void recursive_create(entt::registry& registry, const EntityAssembly entity_assembly, entt::entity parent = entt::null);
	void recursive_destroy(entt::registry& registry, entt::entity entity);

	std::vector<std::function<void(entt::registry&)>> queued_add_components;
	std::vector<std::function<void(entt::registry&)>> queued_remove_components;
	std::vector<entt::entity> queued_destroy;
	std::vector<std::pair<EntityAssembly, entt::entity>> queued_create;
	std::vector<std::function<void(entt::registry&)>> queued_hierarchy_changes;
	size_t reserve_size = 100;
};