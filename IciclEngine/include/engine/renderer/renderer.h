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
#include <engine/resources/data_structs.h>
#include <engine/renderer/frame_buffer.h>

struct DefferedBuffer
{
	FrameBuffer* gbuffer;
	FrameBuffer* output;
};

struct ShadowBuffer
{
	glm::mat4 position;
	FrameBuffer* shadow_map;
};

struct Renderer
{
	~Renderer()
	{
		if (lighting_quad_vbo != 0)
		{
			glDeleteBuffers(1, &lighting_quad_vbo);
		}
		if (lighting_quad_vao != 0)
		{
			glDeleteBuffers(1, &lighting_quad_vao);

		}
	}
	float rotation = 0;
	std::weak_ptr<ShaderProgram> shader_program;
	void temp_render(MeshData& a_mesh, TransformDynamicComponent& a_world_pos);
	void temp_render(RenderRequest& a_render_request, glm::vec3 a_camera_positon = glm::vec3(0));
	void temp_render(RenderContext& a_render_context);
	void temp_set_shader(std::weak_ptr<ShaderProgram> a_shader);

	void deffered_render(RenderContext& a_render_context, const DefferedBuffer& a_deffered_buffers);

	void set_camera_position(glm::vec3 a_position) { camera_position = a_position; };
	void set_proj_view_matrix(glm::mat4 a_proj, glm::mat4 a_view);
	void set_lighting_shader(const std::string& a_shader_path);

	void initialize();
private:


	void bind_uniform(std::type_index a_type, const std::string& a_location, UniformValue a_value_ptr); // good enough, lowers complexity
	void render_lighting_quad(GLuint a_output_tex);
	void generate_lighting_quad();
	void generate_lighting_shader();

	void set_vec1f(const float value, const char* location) const;
	void set_vec2f(const float value[2], const char* location) const;
	void set_vec3f(const float value[3], const char* location) const;
	void set_vec4f(const float value[4], const char* location) const;
	void set_vec1fv(const std::vector<glm::vec1>& a_value, GLsizei a_count, const char* a_location) const;
	void set_vec2fv(const std::vector<glm::vec2>& a_value, GLsizei a_count, const char* a_location) const;
	void set_vec3fv(const std::vector<glm::vec3>& a_value, GLsizei a_count, const char* a_location) const;
	void set_vec4fv(const std::vector<glm::vec4>& a_value, GLsizei a_count, const char* a_location) const;

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
	GLuint current_gl_program = 0;
	bool not_bound = true;

	GLuint lighting_quad_vao = 0;
	GLuint lighting_quad_vbo = 0;

	GLuint lighting_program = 0;

	std::string lighting_shader_path = "./assets/shaders/engine/lighting.shdr";
	unsigned int count = 0;
	glm::vec3 camera_position;
	glm::mat4 proj;
	glm::mat4 view;

	float directional_shadow_resolution = 2048;
	float rect_shadow_resoltuion = 512;
	float spot_shadow_resulution = 512;
	std::vector<FrameBuffer*> directional_shadow_maps;
	std::vector<FrameBuffer*> rect_shadow_maps;
	std::vector<FrameBuffer*> spot_shadow_maps;
	//std::vector<FrameBuffer*> point_shadow_maps;
};

