//#include <engine/resources/material_loader.h>
//#include <nlohmann/json.hpp>
//#include <fstream>
//#include <engine/utilities/macros.h>
//using json = nlohmann::json;
//
//MaterialData load_from_path(const std::string& a_path)
//{
//    MaterialData material_data;
//    // now I need to deserialize a material... which means I need to fix saving...
//	std::ifstream file(a_path);
//	if (!file)
//	{
//		PRINTLN("Failed to load at: {}", a_path);
//	}
//
//	json j;
//	file >> j;
//	material_data.vertex_path = j["vertex_shader"].get<std::string>();
//	material_data.frag_path = j["frag_shader"].get<std::string>();
//	initialize();
//	name = hashed_string_64(j["name"].get<std::string>().c_str());
//	if (j.contains("uniforms") && j["uniforms"].is_array())
//	{
//		uniform_calls.clear();
//		for (auto& j_uniform : j["uniforms"])
//		{
//			UniformData uniform;
//			if (auto serializer = FieldSerializationRegistry::instance().get_serializer(typeid(UniformData)))
//			{
//				serializer.value().deserializable_function(j_uniform, &uniform);
//				uniform_calls.push_back(uniform);
//				//PRINTLN("types: {} {}", uniform.value.type().name(), uniform.type.name());
//			}
//		}
//	}
//	return true;
//    return MaterialData();
//}
