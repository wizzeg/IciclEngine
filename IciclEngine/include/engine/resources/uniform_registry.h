//#pragma once
//#include <functional>
//#include <string>
//#include <unordered_map>
//#include <typeindex>
//#include <glad/glad.h>
//#include <optional>
//#include <engine/utilities/macros.h>
//
//using UniformFunction = std::function<UniformCallEntry>(GLuint, std::string, void*); // don't I also need the shader program number?
//
//// for the stored infor
//struct UniformCallEntry
//{
//	std::type_index type; // glm::mat4 - don't think I need this, but good to keep for safety
//	UniformFunction uniform_call;
//};
//
//struct UniformCallRegistry
//{
//	static UniformCallRegistry& instance()
//	{
//		static UniformCallRegistry instance;
//		return instance;
//	}
//
//	void register_uniform(std::type_index a_type, UniformCallEntry a_function)
//	{
//		if (!uniform_calls.contains(a_type))
//		{
//			uniform_calls[a_type] = a_function;
//		}
//		else
//		{
//			PRINTLN("Attempted to register duplicate uniform function calls");
//		}
//	}
//
//	// we don't need to call this every time, 
//	std::optional<UniformCallEntry> get_uniform_call(std::type_index a_type)
//	{
//		if (uniform_calls.contains(a_type))
//		{
//			return uniform_calls[a_type];
//		}
//		return std::nullopt;
//	}
//
//	bool has_uniform_call(std::type_index a_type)
//	{
//		if (uniform_calls.contains(a_type))
//		{
//			return true;
//		}
//		return false;
//	}
//
//private:
//	UniformCallRegistry() = default;
//	std::unordered_map<std::type_index, UniformCallEntry> uniform_calls; // we do not need to use this for every single call.
//};