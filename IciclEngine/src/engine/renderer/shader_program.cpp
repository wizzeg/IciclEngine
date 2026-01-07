#include <engine/renderer/shader_program.h>
#include <glm/gtc/type_ptr.hpp>
#include <engine/utilities/macros.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <engine/editor/field_serialization_registry.h>

ShaderProgram::ShaderProgram(const char* a_vertex_path, const char* a_frag_path) : vertex_path(a_vertex_path), fragment_path(a_frag_path)
{
	initialize();
}

ShaderProgram::~ShaderProgram()
{
	glUseProgram(0);
	if (shader_program != 0)
		glDeleteProgram(shader_program);
}

bool ShaderProgram::load(std::string a_path)
{
	// for loading, send a pointer to a UniformData ... then the deserializer basically constructs the uniform.
	std::ifstream file(a_path);
	if (!file)
	{
		PRINTLN("Failed to load at: {}", a_path);
		return false;
	}

	json j;
	file >> j;
	vertex_path = j["vertex_shader"].get<std::string>();
	fragment_path = j["frag_shader"].get<std::string>();
	initialize();
	name = hashed_string_64(j["name"].get<std::string>().c_str());
	if (j.contains("uniforms") && j["uniforms"].is_array())
	{
		uniform_calls.clear();
		for (auto& j_uniform : j["uniforms"])
		{
			UniformData uniform;
			if (auto serializer = FieldSerializationRegistry::instance().get_serializer(typeid(UniformData)))
			{
				serializer.value().deserializable_function(j_uniform, &uniform);
				uniform_calls.push_back(uniform);
				//PRINTLN("types: {} {}", uniform.value.type().name(), uniform.type.name());
			}
		}
	}
	return true;
}

bool ShaderProgram::save(std::string a_path)
{
	json j = json::object();
	j["uniforms"] = json::array();
	j["name"] = name.string;
	j["vertex_shader"] = vertex_path;
	j["frag_shader"] = fragment_path;
	for (auto& uniform_call : uniform_calls)
	{
		json j_obj = json::object();
		if (FieldSerializationRegistry::instance().is_serializable(typeid(UniformData)))
		{
			std::string test = uniform_call.type.name();
			auto serializer = FieldSerializationRegistry::instance().get_serializer(typeid(UniformData));
			serializer.value().serializable_function(j_obj, &uniform_call);
		}
		else
		{
			PRINTLN("failed serialization of uniform call");
		}
		j["uniforms"].push_back(j_obj);
	}

	std::ofstream file(a_path);
	if (!file)
	{
		PRINTLN("Failed to save at: {}", a_path);
		return false;
	}
	file << j.dump(4);  // Pretty-print with 4-space indent
	return true;
}

void ShaderProgram::bind_uniform(std::type_index a_type, const std::string& a_location, void* a_value_ptr) // good enough, lowers complexity
{
	// instead do an immediate cast and have this by a type T thing for immediate call
	if (a_type == typeid(glm::vec3))
	{
		set_vec3f(static_cast<const float*>(a_value_ptr), a_location.c_str());
	}
	else if (a_type == typeid(glm::vec4))
	{
		set_vec4f(static_cast<const float*>(a_value_ptr), a_location.c_str());
	}
	else if (a_type == typeid(int))
	{
		set_vec1i(*static_cast<int*>(a_value_ptr), a_location.c_str());
	}
	else if (a_type == typeid(glm::mat4))
	{
		set_mat4fv(*reinterpret_cast<glm::mat4*>(a_value_ptr), a_location.c_str());
	}
}

void ShaderProgram::initialize()
{
	GLuint vertex_shader = load_vertex_shader_from_file(vertex_path.c_str());
	GLuint fragment_shader = load_fragment_shader_from_file(fragment_path.c_str());

	shader_program = glCreateProgram();

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
		PRINTLN("failed to load shader, {} {}", vertex_path, fragment_path);
	}

	glUseProgram(shader_program);

	///////////////////////// REMOVE THIS LATER /////////////////////////////////////// can kinda do this though as start anyway
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 10.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::mat4 view;
	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)640 / (float)480, 0.1f, 100.0f);
	set_mat4fv(view, "view");
	set_mat4fv(proj, "proj");
	glUseProgram(0);
}

GLuint ShaderProgram::load_vertex_shader_from_file(const char* a_path)
{
	return load_shader_from_file(a_path, GL_VERTEX_SHADER);
}

GLuint ShaderProgram::load_fragment_shader_from_file(const char* a_path)
{
	return load_shader_from_file(a_path, GL_FRAGMENT_SHADER);
}

GLuint ShaderProgram::load_shader_from_file(const char* a_path, GLenum a_shader_type)
{
	std::string shaderCodeString = load_string_from_file(a_path);
	const char* shaderCode = shaderCodeString.c_str();

	GLuint shader_object;
	shader_object = glCreateShader(a_shader_type);
	glShaderSource(shader_object, 1, &shaderCode, NULL);
	glCompileShader(shader_object);

	int result;
	glGetShaderiv(shader_object, GL_COMPILE_STATUS, &result);

	if (!result)
	{
		char log[512];
		std::string type;
		switch (a_shader_type)
		{
		case GL_VERTEX_SHADER:
			type = "Vertex Shader";
			break;
		case GL_FRAGMENT_SHADER:
			type = "Fragment Shader";
			break;
		default:
			type = "Unknown";
			break;
		}
		glGetShaderInfoLog(shader_object, 512, NULL, log);
		PRINTLN("Failed to compile: {}, {}", type, log);
	}
	return shader_object;
}

std::string ShaderProgram::load_string_from_file(const char* a_path)
{
	std::ifstream file(a_path);
	if (!file.is_open())
	{
		PRINTLN("Failed to open file: {}", a_path);
		return "";
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

void ShaderProgram::set_vec1f(const float a_value, const char* a_location) const
{
	glUniform1f(glGetUniformLocation(shader_program, a_location), a_value);
}
void ShaderProgram::set_vec2f(const float a_value[2], const char* a_location) const
{
	glUniform2f(glGetUniformLocation(shader_program, a_location), a_value[0], a_value[1]);
}
void ShaderProgram::set_vec3f(const float a_value[3], const char* a_location) const
{
	glUniform3f(glGetUniformLocation(shader_program, a_location), a_value[0], a_value[1], a_value[2]);
}
void ShaderProgram::set_vec4f(const float a_value[4], const char* a_location) const
{
	glUniform4f(glGetUniformLocation(shader_program, a_location), a_value[0], a_value[1], a_value[2], a_value[3]);
}

void ShaderProgram::set_vec1i(const int a_value, const char* a_location) const
{
	glUniform1i(glGetUniformLocation(shader_program, a_location), a_value);
}
void ShaderProgram::set_vec2i(const int a_value[2], const char* a_location) const
{
	glUniform2i(glGetUniformLocation(shader_program, a_location), a_value[0], a_value[1]);
}
void ShaderProgram::set_vec3i(const int a_value[3], const char* a_location) const
{
	glUniform3i(glGetUniformLocation(shader_program, a_location), a_value[0], a_value[1], a_value[2]);
}
void ShaderProgram::set_vec4i(const int a_value[4], const char* a_location) const
{
	glUniform4i(glGetUniformLocation(shader_program, a_location), a_value[0], a_value[1], a_value[2], a_value[3]);
}

void ShaderProgram::set_vec1ui(const unsigned int a_value, const char* a_location) const
{
	glUniform1ui(glGetUniformLocation(shader_program, a_location), a_value);
}
void ShaderProgram::set_vec2ui(const unsigned int a_value[2], const char* location) const
{
	glUniform2ui(glGetUniformLocation(shader_program, location), a_value[0], a_value[1]);
}
void ShaderProgram::set_vec3ui(const unsigned int a_value[3], const char* a_location) const
{
	glUniform3ui(glGetUniformLocation(shader_program, a_location), a_value[0], a_value[1], a_value[2]);
}
void ShaderProgram::set_vec4ui(const unsigned int a_value[4], const char* a_location) const
{
	glUniform4ui(glGetUniformLocation(shader_program, a_location), a_value[0], a_value[1], a_value[2], a_value[3]);
}
void ShaderProgram::set_mat4fv(const glm::mat4 a_value, const char* a_location) const
{
	glUniformMatrix4fv(glGetUniformLocation(shader_program, a_location), 1, GL_FALSE, glm::value_ptr(a_value));
}