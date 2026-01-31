#pragma once

#include <engine/utilities/entt_modified.h>
#include <typeindex>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <any>
#include <variant>
#include <string>
#include <unordered_map>
#include <mutex>
#include <engine/utilities/macros.h>
#include <condition_variable>
#include <functional>
//TODO move the stuff for physics to another header, otherwise circular references

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


struct SystemsStorageObjectBase
{
	std::condition_variable cv;
	std::mutex mutex;
	int waiting_readers = 0;
	int readers = 0;
	int waiting_writers = 0;
	bool writing = false;
};


template<typename T>
struct SystemsStorageObject : SystemsStorageObjectBase
{
	SystemsStorageObject(T a_data) : data(a_data) {}
	T data;
	void read(std::function <void(const T&)>&& func)
	{
		std::unique_lock<std::mutex> read_lock(mutex);
		waiting_readers++;
		cv.wait(read_lock, [this]() { return !writing && waiting_writers == 0; }); // maybe remove this
		waiting_readers--;
		readers++;
		read_lock.unlock();
		func(data);
		read_lock.lock();
		readers--;
		cv.notify_all();
	}
	void write(std::function <void(T&)>&& func)
	{
		std::unique_lock write_lock(mutex);
		waiting_writers++;
		cv.wait(write_lock, [this]() { return !writing && readers == 0; });
		waiting_writers--;
		writing = true;
		write_lock.unlock();
		func(data);
		write_lock.lock();
		writing = false;
	}

	void copy(T& data_copy)
	{
		std::unique_lock<std::mutex> read_lock(mutex);
		waiting_readers++;
		cv.wait(read_lock, [this]() { return !writing; });
		waiting_readers--;
		readers++;
		read_lock.unlock();
		data_copy = data;
		read_lock.lock();
		readers--;
		cv.notify_all();
	}
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