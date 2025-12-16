#pragma once

#include <engine/utilities/entt_modified.h>
#include <typeindex>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <any>

struct EntityReference
{
    entt::entity entity = entt::null;
    uint32_t scene_object = 0;
};

struct UniformCall
{
	hashed_string_64 name;
	bool dirty = true;
	std::type_index type = typeid(int);
	void* value_ptr = 0;
};

struct UniformData
{
	hashed_string_64 location;
	std::any value = (int)0;
	std::type_index type = value.type();
	bool dirty = true;
};

//struct UniformDataBase
//{
//	UniformDataBase(const std::string& a_name, std::type_index a_type, void* a_value_ptr) : name(hashed_string_64(a_name.c_str())), type(a_type), value_ptr(a_value_ptr) {};
//	hashed_string_64 name;
//	bool dirty = true;
//	std::type_index type = typeid(int);
//	void* value_ptr = 0;
//};
//
//template<typename TData>
//struct UniformData : UniformDataBase
//{
//	UniformData(const std::string& a_name, TData a_value) : value(a_value), UniformDataBase(a_name, typeid(TData), nullptr) 
//	{ 
//		//std::string name = type.name();
//		//if (name.find("glm"))
//		//{
//		//	value_ptr = glm::value_ptr(value);
//		//}
//		//else
//		//{
//			value_ptr = &value;
//		//}
//		
//	};
//	TData value;
//};