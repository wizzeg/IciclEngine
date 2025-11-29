#pragma once
#include <memory>
#include <vector>

struct FieldInfo;
class SceneObject;

struct UIObjectPropertyDrawer
{
	void draw_object_properties(std::shared_ptr<SceneObject>& a_scene_object);
	void draw_component_fields(std::vector<FieldInfo>& a_field_info);
};

