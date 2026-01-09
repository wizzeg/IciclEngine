#pragma once
#include <engine/renderer/render_info.h>
struct ShaderLoader
{
	ShaderData static load_shader_from_path(const std::string& a_path);
	MaterialData static load_material_from_path(const std::string& a_path);
	GLuint static compile_shader(ShaderData& a_shader);
};

