#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <string>
#include <engine/renderer/render_info.h>

struct ObjParser
{
	MeshData static load_mesh_from_filepath(const std::string& a_path);
};

