#include <engine/renderer/shader_program.h>
#include <glm/gtc/type_ptr.hpp>
#include <engine/utilities/macros.h>
#include <fstream>
#include <sstream>
#include <iostream>

ShaderProgram::ShaderProgram(const char* a_vertex_path, const char* a_frag_path)
{
	GLuint vertex_shader = load_vertex_shader_from_file(a_vertex_path);
	GLuint fragment_shader = load_fragment_shader_from_file(a_frag_path);

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
		PRINTLN("failed to load shader, {} {}", a_vertex_path, a_frag_path);
	}

	glUseProgram(shader_program);

	///////////////////////// REMOVE THIS LATER ///////////////////////////////////////
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 10.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::mat4 view;
	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)640 / (float)480, 0.1f, 100.0f);
	set_mat4fv(view, "view");
	set_mat4fv(proj, "projection");
	glUseProgram(0);
}

ShaderProgram::~ShaderProgram()
{
	glUseProgram(0);
	if (shader_program != 0)
		glDeleteProgram(shader_program);
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