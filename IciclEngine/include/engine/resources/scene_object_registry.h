#pragma once
#include <cstdint>
#include <memory>
#include <unordered_map>
class SceneObject;

struct SceneObjectWptr
{
	std::weak_ptr<SceneObject> scene_object;
	bool is_valid;
};

struct SceneObjectRegistry
{
	uint32_t add_scene_object(std::weak_ptr<SceneObject> a_scene_object_wptr);
	void remove_scene_object(uint32_t a_id);
	std::weak_ptr<SceneObject> id_to_scene_object(uint32_t a_id, bool& exists);

private:
	uint32_t next_free = 0;
	std::unordered_map<uint32_t, SceneObjectWptr> scene_object_map;
};

