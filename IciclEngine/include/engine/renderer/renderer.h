#pragma once
#include <engine/renderer/render_info.h>
#include <engine/renderer/shader_program.h>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <memory>
#include <engine/game/components.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <string>
#include <typeindex>

struct Renderer
{
	float rotation = 0;
	std::weak_ptr<ShaderProgram> shader_program;
	void temp_render(MeshData& a_mesh, TransformDynamicComponent& a_world_pos);
	void temp_render(RenderRequest& a_render_request, glm::vec3 a_camera_positon = glm::vec3(0));
	void temp_render(RenderContext a_render_context);
	void temp_set_shader(std::weak_ptr<ShaderProgram> a_shader);
	void set_camera_position(glm::vec3 a_position) { camera_position = a_position; };
	void set_proj_view_matrix(glm::mat4 a_proj, glm::mat4 a_view);
private:


	void bind_uniform(std::type_index a_type, const std::string& a_location, UniformValue a_value_ptr); // good enough, lowers complexity

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

	GLuint current_gl_program = 0;
	bool not_bound = true;
	unsigned int count = 0;
	glm::vec3 camera_position;
	glm::mat4 proj;
	glm::mat4 view;
};

