#pragma once

#include <engine/utilities/entt_modified.h>
#include <typeindex>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <any>
#include <variant>
#include <string>
using UniformValue = std::variant<bool, int, float, double, glm::vec3, glm::vec4, glm::quat, glm::mat4, glm::ivec1, std::string>;

namespace EShadowCasterType
{
	enum EShadowCasterType
	{
		DirectionalLight,
		RectangularLight,
		SpotLight,
		PointLight
	};
};

struct ShadowLight
{
	glm::mat4 lightspace_matrix = glm::mat4(0);
	glm::vec3 rotation = glm::vec3(1.f);
	glm::vec4 color = glm::vec4(0);
	glm::vec4 attenuation = glm::vec4(0);
	EShadowCasterType::EShadowCasterType type = EShadowCasterType::PointLight;
};

struct alignas(16) PointLightSSBO
{
	alignas(16) glm::vec4 light_color;
	alignas(16) glm::vec4 light_positoin;
	alignas(16) glm::vec4 light_attenuation;
};

struct Light
{
	alignas(16)glm::mat4 model_matrix = glm::mat4(0);
	glm::vec3 color = glm::vec3(0);
	glm::vec3 attenuation = glm::vec3(0);
	float intensity = 0;
	EShadowCasterType::EShadowCasterType type = EShadowCasterType::PointLight;
};

struct EntityReference
{
    entt::entity entity = entt::null;
    uint32_t scene_object = 0;
};

struct UniformTexture
{
	std::string file_path;
	std::string uniform_name;
};

struct UniformData
{
	hashed_string_64 location;
	std::type_index type = typeid(bool);
	UniformValue value = (bool)false;
	GLint texture_id = 0;
	bool added_reference = false;
	uint64_t modified_time; 
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