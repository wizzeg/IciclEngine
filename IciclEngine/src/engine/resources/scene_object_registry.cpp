#include "scene_object_registry.h"
#include "scene_object.h"

uint32_t SceneObjectRegistry::add_scene_object(std::weak_ptr<SceneObject> a_scene_object_wptr)
{
	scene_object_map[next_free] = SceneObjectWptr{ a_scene_object_wptr, (!a_scene_object_wptr.expired()) }; return next_free++;
}

void SceneObjectRegistry::remove_scene_object(uint32_t a_id)
{
	if (scene_object_map.find(a_id) != scene_object_map.end())
	{
		scene_object_map[a_id].is_valid = false;
	}
}

std::weak_ptr<SceneObject> SceneObjectRegistry::id_to_scene_object(uint32_t a_id, bool& exists)
{
	if (scene_object_map.find(a_id) != scene_object_map.end())
	{
		exists = true;
		return scene_object_map[a_id].scene_object;
	}
	else
	{
		exists = false;
		return std::weak_ptr<SceneObject>();
	}
};