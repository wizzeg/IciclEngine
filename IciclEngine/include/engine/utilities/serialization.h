#pragma once
#include <engine/editor/component_registry.h>
#include <engine/editor/field_serialization_registry.h>
#include <engine/editor/field_serialization_macro.h>
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

inline void serialize_field(json& a_json, const std::string& a_field_name, std::type_index a_type, void* a_object)
{
	auto& comp_reg = ComponentRegistry::instance();
	auto& field_serializer = FieldSerializationRegistry::instance();
	if (field_serializer.is_serializable(a_type))
	{
		
		if (auto serializer = field_serializer.get_serializer(a_type))
		{
			json j_field;
			serializer.value().serializable_function(j_field, a_object);
			//a_json["name"] = a_field_name;
			//a_json["type"] = a_type.name();
			a_json["value"] = j_field;
		}
	}

	else if (comp_reg.is_registered(a_type))
	{
		json j_comp = json::object();
		j_comp["fields"] = json::array();
		std::vector<FieldInfo> fields = comp_reg.get_field_info_from_type(a_type, a_object);
		for (auto& field : fields)
		{
			json j_comp_field;
			serialize_field(j_comp_field, field.name, field.type, field.value_ptr);
			j_comp["fields"].push_back(j_comp_field);
		}
		j_comp["type"] = a_field_name;
	}
	else
	{
		PRINTLN("FAILED FIELD");
	}
}

inline void deserialize_field(const json& j, std::type_index a_type, void* a_object)
{
	if (!j.is_object()) return;
	else if (!j.contains("value")) return;
	auto& comp_reg = ComponentRegistry::instance();
	auto& field_serializer = FieldSerializationRegistry::instance();
	if (field_serializer.is_serializable(a_type))
	{
		if (auto deserializer = field_serializer.get_serializer(a_type))
		{
			deserializer.value().deserializable_function(j["value"], a_object);
		}
	}
}
