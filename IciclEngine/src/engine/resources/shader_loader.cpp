#include <engine/renderer/shader_loader.h>
#include <engine/utilities/macros.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <engine/utilities/hashed_string_64.h>
#include <engine/resources/uniform_registry.h>
#include <engine/resources/data_structs.h>
#include <engine/editor/field_serialization_registry.h>

using json = nlohmann::json;

ShaderData ShaderLoader::load_shader_from_path(const std::string& a_path)
{
	ShaderData shader;
	shader.hashed_path = hashed_string_64(a_path.c_str());
	std::ifstream file(a_path);
	if (!file)
	{
		PRINTLN("Failed to load shader at: {}", a_path);
		shader.loading_status = ELoadStatus::FailedLoadBadPath;
		return shader;
	}

	json j;
	file >> j;

	shader.vert_path = j["vert_shader"].get<std::string>();


	std::ifstream vert_file(shader.vert_path);
	if (!vert_file.is_open())
	{
		PRINTLN("Failed to open vert file: {}", a_path);
	}
	{
		std::stringstream buffer;
		buffer << file.rdbuf();
		shader.frag_buffer = buffer.str();
	}


	shader.frag_path = j["frag_shader"].get<std::string>();
	shader.loading_status = ELoadStatus::ShaderLoadedPath;
    return ShaderData();
}

MaterialData ShaderLoader::load_material_from_path(const std::string& a_path)
{
	std::ifstream file(a_path);
	MaterialData material;
	material.hashed_path = hashed_string_64(a_path.c_str());
	if (!file)
	{
		PRINTLN("Failed to load at: {}", a_path);
		material.load_status = ELoadStatus::FailedLoadOpen;
		return material;
	}

	json j;
	file >> j;
	std::string shader_path = j["shader_path"].get<std::string>();

	std::vector<UniformData> uniforms;
	if (j.contains("uniforms") && j["uniforms"].is_array())
	{
		for (auto& j_uniform : j["uniforms"])
		{
			UniformData uniform;
			if (auto serializer = FieldSerializationRegistry::instance().get_serializer(typeid(UniformData)))
			{
				serializer.value().deserializable_function(j_uniform, &uniform);
				uniforms.push_back(uniform);
			}
		}
	}
	return MaterialData();
}
