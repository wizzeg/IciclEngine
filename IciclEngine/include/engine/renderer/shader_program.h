#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <string>

struct ShaderProgram
{
	ShaderProgram(const char* a_vertex_path, const char* a_frag_path);
	~ShaderProgram();

	void bind() const { glUseProgram(shader_program); }
	void recompile() const {}; // later
	void destroy() { glUseProgram(0); glDeleteShader(shader_program); shader_program = 0; };
	void unbind() const { glUseProgram(0); }

	GLuint load_vertex_shader_from_file(const char* a_path);
	GLuint load_fragment_shader_from_file(const char* a_path);
	GLuint load_shader_from_file(const char* a_path, GLenum a_shader_type);
	const GLuint get_shader_program() const { return shader_program; };
	std::string load_string_from_file(const char* a_path);

	void set_vec1f(const float value, const char* location) const;
	void set_vec2f(const float value[2], const char* location) const;
	void set_vec3f(const float value[3], const char* location) const;
	void set_vec4f(const float value[4], const char* location) const;

	void set_vec1i(const int value, const char* location) const;
	void set_vec2i(const int value[2], const char* location) const;
	void set_vec3i(const int value[3], const char* location) const;
	void set_vec4i(const int value[4], const char* location) const;

	void set_vec1ui(const unsigned int value, const char* location) const;
	void set_vec2ui(const unsigned int value[2], const char* location) const;
	void set_vec3ui(const unsigned int value[3], const char* location) const;
	void set_vec4ui(const unsigned int value[4], const char* location) const;

	void set_mat4fv(glm::mat4 value, const char* location) const;



private:
	GLuint shader_program = 0;
};

