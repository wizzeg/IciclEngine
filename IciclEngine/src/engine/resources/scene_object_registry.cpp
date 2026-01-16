#include <engine/resources/scene_object_registry.h>

std::vector<TrueID*> SceneObjectRegistry::get_new_IDs(uint32_t a_num_IDs, bool a_thread_safe)
{
	std::vector<TrueID*> new_IDs;
	new_IDs.reserve(a_num_IDs);
	if (a_thread_safe)
	{
		if (next_id + a_num_IDs >= UINT32_MAX)
		{
			return new_IDs;
		}
		std::lock_guard ID_guard(ID_mutex);
		bool panic = false;
		for (uint32_t i = 0; i < a_num_IDs; i++)
		{
			get_next_id();
			IDs[next_id] = TrueID(next_id);
			new_IDs.emplace_back(&IDs[next_id]);
		}
		return new_IDs;
	}
	new_IDs.reserve(a_num_IDs);
	if (next_id + a_num_IDs >= UINT32_MAX)
	{
		return new_IDs;
	}
	std::lock_guard ID_guard(ID_mutex);
	bool panic = false;
	for (uint32_t i = 0; i < a_num_IDs; i++)
	{
		get_next_id();
		IDs[next_id] = TrueID(next_id);
		new_IDs.emplace_back(&IDs[next_id]);
	}
	return new_IDs;
}

TrueID& SceneObjectRegistry::get_new_ID(entt::entity a_entity, bool a_thread_safe)
{
	if (a_thread_safe)
	{
		std::lock_guard ID_guard(ID_mutex);
		TrueID id(get_next_id(), a_entity, (a_entity != entt::null));
		IDs[id.true_id] = id;
		return IDs[id.true_id];
	}
	TrueID id(get_next_id(), a_entity, (a_entity != entt::null));
	IDs[id.true_id] = id;
	return IDs[id.true_id];
}

TrueID& SceneObjectRegistry::get_requested_ID(uint32_t a_requested_id, entt::entity a_entity, bool a_thread_safe)
{
	if (a_thread_safe)
	{
		std::lock_guard ID_guard(ID_mutex);
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

TrueID* SceneObjectRegistry::get_registred_ID(uint32_t a_id, bool a_thread_safe)
{
	if (a_thread_safe)
	{
		std::lock_guard ID_guard(ID_mutex);
		if (!IDs.contains(a_id))
		{
			return nullptr;
		}
		return &IDs[a_id];
	}
	if (!IDs.contains(a_id))
	{
		return nullptr;
	}
	return &IDs[a_id];
}


void SceneObjectRegistry::clear_registry(bool a_thread_safe)
{
	if (a_thread_safe)
	{
		std::lock_guard ID_guard(ID_mutex);
		next_id = 1;
		IDs.clear();
	}
	else
	{
		next_id = 1;
		IDs.clear();
	}
};

uint32_t SceneObjectRegistry::get_next_id()
{

	while (IDs.contains(next_id))
	{
		if (next_id == UINT32_MAX) // not sure whatto do now...
		{
			break;
		}
		next_id++;
	}
	return next_id;
}
