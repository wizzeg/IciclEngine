#include <engine/core/entity_command_buffer.h>
#include <engine/resources/scene_object_registry.h>

EntityAssembler& EntityAssembler::with_child(EntityAssembly child, const std::string& name)
{
	// have to do work to work in editor too
	entity_assembly.children.push_back(std::move(child));
	entity_assembly.children.back().name = name;
	return *this;
}


void EntityCommandBuffer::create_entity(EntityAssembly entity_assembly, entt::entity parent, const std::string& name)
{
	queued_create.push_back({ std::move(entity_assembly), parent });
	queued_create.back().first.name = name;
}

void EntityCommandBuffer::create_entity(EntityAssembly entity_assembly, const std::string& name)
{
	queued_create.push_back({ std::move(entity_assembly), entt::null });
	queued_create.back().first.name = name;
}

void EntityCommandBuffer::set_parent(entt::entity parent, entt::entity child)
{
	queued_hierarchy_changes.push_back([parent, child](entt::registry& registry)
		{
			if (registry.valid(parent) && registry.valid(child))
			{
				// could also copy the true_id later too
				// fix old parenting
				if (auto child_refs = registry.try_get<HierarchyComponent>(child))
				{
					if (registry.valid(child_refs->parent.entity))
					{
						auto old_parent_refs = registry.try_get<HierarchyComponent>(child_refs->parent.entity);
						if (old_parent_refs && old_parent_refs->child.entity == child)
						{
							old_parent_refs->child = child_refs->next_sibling;
						}
					}
					if (registry.valid(child_refs->previous_sibling.entity))
					{
						if (auto prev_refs = registry.try_get<HierarchyComponent>(child_refs->previous_sibling.entity))
						{
							prev_refs->next_sibling = child_refs->next_sibling;
						}
					}
					if (registry.valid(child_refs->next_sibling.entity))
					{
						if (auto next_refs = registry.try_get<HierarchyComponent>(child_refs->next_sibling.entity))
						{
							next_refs->previous_sibling = child_refs->previous_sibling;
						}
					}
					child_refs->previous_sibling = EntityReference{};
					child_refs->next_sibling = EntityReference{};
					child_refs->parent = EntityReference{};
					// set new parenting
					if (auto parent_refs = registry.try_get<HierarchyComponent>(parent))
					{
						if (parent_refs->child.entity != entt::null)
						{
							auto& old_child_refs = registry.get<HierarchyComponent>(parent_refs->child.entity);
							auto& old_child_entity = registry.get<EntityComponent>(parent_refs->child.entity);
							old_child_refs.previous_sibling.entity = child;
							child_refs->next_sibling.entity = old_child_entity.entity.entity;
						}
						parent_refs->child.entity = child;

					}
				}
			}
		});
}

void EntityCommandBuffer::orphan(entt::entity entity)
{
	// orphan, should be quick enough
	queued_hierarchy_changes.push_back([entity](entt::registry& registry)
		{
			if (registry.valid(entity))
			{
				if (auto ent_refs = registry.try_get<HierarchyComponent>(entity))
				{
					HierarchyComponent new_hier;
					new_hier.child = ent_refs->child;
					auto parent = ent_refs->parent.entity;
					if (registry.valid(parent))
					{
						if (auto parent_refs = registry.try_get<HierarchyComponent>(parent))
						{
							// case first child
							if (entity == parent_refs->child.entity)
							{
								parent_refs->child = ent_refs->next_sibling;
								if (registry.valid(ent_refs->next_sibling.entity))
								{
									if (auto next_refs = registry.try_get<HierarchyComponent>(ent_refs->next_sibling.entity))
									{
										next_refs->previous_sibling = EntityReference{};
									}
								}
							}
							// case middle child
							else
							{
								if (registry.valid(ent_refs->previous_sibling.entity))
								{
									if (auto prev_refs = registry.try_get<HierarchyComponent>(ent_refs->previous_sibling.entity))
									{
										prev_refs->next_sibling = ent_refs->next_sibling;
									}
								}
								if (registry.valid(ent_refs->next_sibling.entity))
								{
									if (auto next_refs = registry.try_get<HierarchyComponent>(ent_refs->next_sibling.entity))
									{
										next_refs->previous_sibling = ent_refs->previous_sibling;
									}
								}
							}
						}
					}
					*ent_refs = new_hier;
				}
			}
		});
}

void EntityCommandBuffer::destroy_entity(entt::entity entity)
{
	queued_destroy.push_back(entity);
}

void EntityCommandBuffer::execute_queue(entt::registry& registry)
{
	for (auto& entity : queued_destroy)
	{
		auto& entity_refs = registry.get<HierarchyComponent>(entity);
		recursive_destroy(registry, entity_refs.child.entity);
		if (registry.valid(entity_refs.parent.entity))
		{
			if (auto* parent_refs = registry.try_get<HierarchyComponent>(entity_refs.parent.entity))
			{
				if (parent_refs->child.entity == entity)
				{
					parent_refs->child.entity = entity_refs.next_sibling.entity;
				}
			}
		}
		if (registry.valid(entity_refs.next_sibling.entity))
		{
			if (auto* next_refs = registry.try_get<HierarchyComponent>(entity_refs.next_sibling.entity))
			{
				next_refs->previous_sibling.entity = entity_refs.previous_sibling.entity;
			}
		}
		if (registry.valid(entity_refs.previous_sibling.entity))
		{
			if (auto* prev_refs = registry.try_get<HierarchyComponent>(entity_refs.previous_sibling.entity))
			{
				prev_refs->next_sibling.entity = entity_refs.next_sibling.entity;
			}
		}
		registry.destroy(entity);
	}
	queued_destroy.clear();

	for (auto& entity_assembly : queued_create)
	{
		recursive_create(registry, entity_assembly.first, entity_assembly.second);
	}
	queued_create.clear();

	for (auto& add_component : queued_add_components)
	{
		add_component(registry);
	}
	queued_add_components.clear();

	for (auto& remove_component : queued_remove_components)
	{
		remove_component(registry);
	}
	queued_remove_components.clear();

	for (auto& hierarchy_change : queued_hierarchy_changes)
	{
		hierarchy_change(registry);
	}
	queued_hierarchy_changes.clear();
}

void EntityCommandBuffer::recursive_create(entt::registry& registry, const EntityAssembly entity_assembly, entt::entity parent)
{
	entt::entity entity = registry.create();
	// it's too bad that I have put a hashed string into here... but too much work to remove now
	registry.emplace<EntityComponent>(entity, entity_assembly.name.c_str(), EntityReference{ entity, 0});
	for (auto& comp_modifier : entity_assembly.component_modifiers)
	{
		comp_modifier(registry, entity);
	}
	if (parent != entt::null)
	{
		auto& parent_entity = registry.get<EntityComponent>(parent);
		auto& parent_refs = registry.get<HierarchyComponent>(parent);
		HierarchyComponent entity_refs;
		entity_refs.parent.entity = parent;
		if (parent_refs.child.entity != entt::null)
		{
			auto& child_refs = registry.get<HierarchyComponent>(parent_refs.child.entity);
			auto& child_entity = registry.get<EntityComponent>(parent_refs.child.entity);
			child_refs.previous_sibling.entity = entity;
			entity_refs.next_sibling.entity = child_entity.entity.entity;
		}
		parent_refs.child.entity = entity;
		registry.emplace<HierarchyComponent>(entity, entity_refs);
	}
	else
	{
		registry.emplace<HierarchyComponent>(entity);
	}

	for (auto& child_assembly : entity_assembly.children)
	{
		recursive_create(registry, child_assembly, entity);
	}
}
void EntityCommandBuffer::recursive_destroy(entt::registry& registry, entt::entity entity)
{
	if (!registry.valid(entity))
		return;

	if (auto* hierarchy = registry.try_get<HierarchyComponent>(entity))
	{
		entt::entity child = hierarchy->child.entity;
		while (child != entt::null)
		{
			entt::entity next_sibling = entt::null;
			if (auto* child_hierarchy = registry.try_get<HierarchyComponent>(child))
			{
				next_sibling = child_hierarchy->next_sibling.entity;
			}
			recursive_destroy(registry, child);
			child = next_sibling;
		}
	}
	registry.destroy(entity);
}