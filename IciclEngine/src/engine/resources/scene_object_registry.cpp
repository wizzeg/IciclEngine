#include <engine/resources/scene_object_registry.h>
#include <engine/editor/scene_object.h>

TrueID& SceneObjectRegistry::get_new_ID(entt::entity a_entity)
{
	TrueID id(get_next_id(), a_entity, (a_entity != entt::null));
	IDs[id.true_id] = id;
	return IDs[id.true_id];
}

TrueID& SceneObjectRegistry::get_requested_ID(uint32_t a_requested_id, entt::entity a_entity)
{
	if (!IDs.contains(a_requested_id))
	{
		TrueID id(a_requested_id, a_entity, (a_entity != entt::null));
		IDs[id.true_id] = id;
		return IDs[id.true_id];
	}
	TrueID no_id;
	IDs[no_id.true_id] = no_id;
	return IDs[no_id.true_id];
}

uint32_t SceneObjectRegistry::get_next_id()
{
	while (!IDs.contains(next_id))
	{
		next_id++;
	}
	return next_id;
}
