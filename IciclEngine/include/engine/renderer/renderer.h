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

struct Renderer
{
	float rotation = 0;
	std::weak_ptr<ShaderProgram> shader_program;
	void temp_render(MeshData& a_mesh, TransformDynamicComponent& a_world_pos);
	void temp_render(RenderRequest& a_render_request, glm::vec3 a_camera_positon = glm::vec3(0));
	void temp_set_shader(std::weak_ptr<ShaderProgram> a_shader);
	void set_proj_view_matrix(glm::mat4 a_proj, glm::mat4 a_view);
private:
	bool not_bound = true;
	unsigned int count = 0;
	glm::mat4 proj;
	glm::mat4 view;
};

