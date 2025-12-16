#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <string>
#include <typeindex>
#include <engine/resources/uniform_registry.h>
#include <engine/resources/data_structs.h>
#include <engine/utilities/macros.h>
#include <string>
#include <engine/utilities/hashed_string_64.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>


struct ShaderProgram
{
	ShaderProgram() {};
	ShaderProgram(const std::string& a_path) { load(a_path); };
	ShaderProgram(const char* a_vertex_path, const char* a_frag_path);
	~ShaderProgram();
	bool load(std::string a_path);
	bool save(std::string a_path);
	void bind() const { glUseProgram(shader_program); }
	void recompile() const {}; // later
	void destroy() { glUseProgram(0); glDeleteShader(shader_program); shader_program = 0; };
	void unbind() const { glUseProgram(0); }

	void bind_uniform(std::type_index a_type, const std::string& a_location, void* a_value_ptr); // just do a bunch of if else if... I don't hve to run this often anyway

	void initialize();
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
	std::string vertex_path = "vertex path";
	std::string fragment_path = "fragment path";
	hashed_string_64 name = hashed_string_64("shader");
	glm::vec3 test = glm::vec3(1);
	std::vector<UniformData> uniform_calls{ 
		{UniformData{"some value", glm::vec3(1.0f, 2.0f, 3.f) }},
		{UniformData{"some value", glm::mat4(1.0f) }},
		{UniformData{"some value", glm::ivec1(5) }}
	}; // sort this after loading, so that materials can be safety checked that they don't have any targets that don't exist.
	GLuint shader_program = 0;
};

