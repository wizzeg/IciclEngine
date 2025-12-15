#pragma once
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <engine/utilities/entt_modified.h>
class SceneObject;

struct TrueID
{
	uint32_t true_id = 0;
	entt::entity entity = entt::null;
	bool initialized = false;
};

struct SceneObjectRegistry
{
	// problem is if multiple scenes are loaded ontop of each other...they'll have overlapping IDs.
	// load scene into a container first... so place load/save logic outside of scene -> create a new map <uint32_t, uint32_t>
	// use this map to map old ID to new ID. Lots of random access, but it'll just have to do.
	TrueID& get_new_ID(entt::entity a_entity = entt::null);
	TrueID& get_requested_ID(uint32_t a_requested_id, entt::entity a_entity);
private:
	uint32_t get_next_id();
	uint32_t next_id = 1;
	std::unordered_map<uint32_t, TrueID> IDs; ///this should be the one, a "scene_object"/"true id" to entity
		// even if there are no scene_objects (game build), they'll still have a "true id" to reference, and stored in NameComponent EntityReference.
}; // I'll need an extra mapping for during load, to do retargeting.

