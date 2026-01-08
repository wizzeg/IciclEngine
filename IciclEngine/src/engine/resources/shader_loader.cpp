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
	shader.loading_status = ELoadStatus::StartedLoad;
	std::ifstream file(a_path);
	if (!file)
	{
		PRINTLN("Failed to load shader at: {}", a_path);
		shader.loading_status = ELoadStatus::FailedLoadBadPath;
		return shader;
	}

	json j = json::object();
	file >> j;

	shader.vert_path = j["vert_path"].get<std::string>();
	std::ifstream vert_file(shader.vert_path);
	if (!vert_file.is_open())
	{
		PRINTLN("Failed to open vert file: {}", a_path);
		shader.loading_status = ELoadStatus::FailedLoadBadPath;
	}
	else
	{
		std::stringstream buffer;
		buffer << vert_file.rdbuf();
		shader.vert_buffer = buffer.str();
	}

	shader.frag_path = j["frag_path"].get<std::string>();
	std::ifstream frag_file(shader.frag_path);
	if (!frag_file.is_open())
	{
		PRINTLN("Failed to open vert file: {}", a_path);
		shader.loading_status = ELoadStatus::FailedLoadBadPath;
	}
	else
	{
		std::stringstream buffer;
		buffer << frag_file.rdbuf();
		shader.frag_buffer = buffer.str();
	}
	if (shader.loading_status != ELoadStatus::FailedLoadBadPath)
	{
		shader.loading_status = ELoadStatus::ShaderLoadedPath;
	}
    return shader;
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

	json j = json::object();
	file >> j;
	std::string shader_path = j.value("shader_path","");
	material.shader_path = hashed_string_64(shader_path.c_str());
	material.is_lit = j.value("lit", false);
	material.instanced = j.value("instanced", false);
	material.recieves_shadows = j.value("recieves_shadows", false);
	material.casts_shadows = j.value("casts_shadows", false);
	material.transparent = j.value("transparent", false);
	material.gl_program = 0;
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
	material.uniforms = uniforms;
	return material;
}

GLint ShaderLoader::compile_shader(ShaderData& a_shader)
{
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	{
		
		const char* new_vert_buffer = a_shader.vert_buffer.c_str();
		glShaderSource(vertex_shader, 1, &new_vert_buffer, NULL);
		glCompileShader(vertex_shader);

		int result;
		glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &result);
		if (!result)
		{
			char log[512];
			glGetShaderInfoLog(vertex_shader, 512, NULL, log);
			PRINTLN("Failed to compile vertex shader: {}", log);
		}
		const char* new_frag_buffer = a_shader.frag_buffer.c_str();
		glShaderSource(fragment_shader, 1, &new_frag_buffer, NULL);
		glCompileShader(fragment_shader);

		glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &result);
		if (!result)
		{
			char log[512];
			glGetShaderInfoLog(fragment_shader, 512, NULL, log);
			PRINTLN("Failed to compile vertex shader: {}", log);
		}
	}

	GLint shader_program = glCreateProgram();

	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	int result;
	glGetProgramiv(shader_program, GL_LINK_STATUS, &result);
	if (!result)
	{
		char log[512];
		glGetProgramInfoLog(shader_program, 512, NULL, log);
		PRINTLN("failed to load shader, {} {}, with log: {}", a_shader.vert_path, a_shader.frag_path, log);
	}
	return shader_program;
}
