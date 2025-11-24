#pragma once
#include <engine/renderer/render_info.h>
#include <engine/renderer/shader_program.h>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <memory>
#include <engine/game/components.h>

struct Renderer
{
	float rotation = 0;
	std::weak_ptr<ShaderProgram> shader_program;
	void temp_render(MeshData& a_mesh, WorldPositionComponent& a_world_pos);
	void temp_render(RenderRequest& a_render_request);
	void temp_set_shader(std::weak_ptr<ShaderProgram> a_shader);
};

