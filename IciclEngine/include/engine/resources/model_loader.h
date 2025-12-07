#pragma once

#include <engine/renderer/render_info.h>

struct ObjPosition
{
	glm::vec3 vec3;
	float x, y, z;
};

struct ObjColor
{
	glm::vec3 vec3;
	float r, g, b;
};

struct ObjNormal
{
	glm::vec3 vec3;
	float x, y, z;
};
struct ObjUVs
{
	glm::vec3 vec3;
	float x, y, z;
};

struct ObjFaceVertex
{
	glm::uvec3 vec3;
	GLuint pos, nrm, uv;
};

struct ObjFace
{
	std::vector<ObjFaceVertex> indicies;
};

struct ObjVertex
{
	unsigned int VAO;
	unsigned int VBOs[4]; // position, color, normal, uv
	unsigned int EBO;

	ObjPosition position;
	ObjNormal normal;
	ObjUVs uv;
	ObjFace face;
	bool filled;
};

struct ModelLoader
{
	MeshData load_obj_mesh_from_file(const std::string a_path);
	void load_texture_from_file(TextureData& a_texture_data);
	TextureData load_texture_from_file(const std::string a_path, bool a_mipmap = true);
	TextureData load_texture_from_file(const std::string a_path, const GLenum a_wrap_x, const GLenum a_wrap_y, bool a_mipmap = true);
	TextureData load_texture_from_file(const std::string a_path, const GLenum a_wrap_x, const GLenum a_wrap_y, const GLenum a_filtering_min, const GLenum a_fintering_mag, bool a_mipmap = true);
	TextureData load_texture_from_file(const std::string a_path, const GLenum a_wrap_x, const GLenum a_wrap_y, const GLenum a_filtering_min, const GLenum a_fintering_mag, const GLenum a_mipmap_filtering, bool a_mipmap = true);
};

