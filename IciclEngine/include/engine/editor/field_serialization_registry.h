#pragma once

#include <functional>
#include <unordered_map>
#include <typeindex>
#include <nlohmann/json.hpp>
#include <engine/utilities/macros.h>


using json = nlohmann::json;
using SerializingFunction = void(*)(json&, const void*); // where to write
using DeserializingFunction = void(*)(const json&, void*); // where to read

struct SerializerInfo
{
	SerializingFunction serializable_function;
	DeserializingFunction deserializable_function;
};

struct FieldSerializationRegistry
{
	static FieldSerializationRegistry& instance()
	{
		static FieldSerializationRegistry instance;
		return instance;
	}

	void register_serializable_field(std::type_index a_type, SerializingFunction a_serializable, DeserializingFunction a_deserializable)
	{
		auto it = serializables.find(a_type);
		if (it == serializables.end())
		{
			serializables.emplace(a_type, SerializerInfo(a_serializable, a_deserializable));
		}
		else
		{
			PRINTLN("There already exists a serializable info for type: {} - Do not register it twice!", a_type.name());
		}
	}

	bool is_serializable(std::type_index a_type)
	{
		auto it = serializables.find(a_type);
		return (it != serializables.end());
	}

	std::optional<SerializerInfo> get_serializer(std::type_index a_type)
	{
		auto it = serializables.find(a_type);
		if (it != serializables.end())
		{
			return it->second;
		}
		return std::nullopt;
	}
private:
	FieldSerializationRegistry() = default;
	std::unordered_map<std::type_index, std::string> type_to_name;
	std::unordered_map<std::string, std::type_index> name_to_type;
	std::unordered_map<std::type_index, SerializerInfo> serializables; // I think this has to be string.
};