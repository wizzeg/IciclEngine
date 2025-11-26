#pragma once
#include <string>
#include <vector>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glad/glad.h>
#ifndef MAX_UVS_COLORS
#define MAX_UVS_COLORS 6;
#endif

struct VAORequest
{
	uint32_t mesh_id;
	entt::hashed_string hashed_path;
};

struct LoadRequest
{
	std::string path;
};


struct RenderRequest
{
	GLuint vao = 0;
	GLsizei indices_size = 0;
	glm::mat4 model_matrix = glm::mat4(0);
	GLuint shader_program = 0;
	uint32_t material_id = 0; // I don't know

};

enum BufferAttributeLocation
{
	Position = 0,
	Normal = 1,
	Tangent = 2,
	Bitangent = 3,
	Color0 = 4,
	Color1 = 5,
	Color2 = 6,
	Color3 = 7,
	Color4 = 8,
	Color5 = 9,
	UV0 = 10,
	UV1 = 11,
	UV2 = 12,
	UV3 = 13,
	UV4 = 14,
	UV5 = 15
};

struct MeshHandle
{
	std::uint32_t id;
};

struct MaterialHandle
{
	std::uint32_t id;
};

struct MeshPath
{
	std::string path;
};

struct MaterialPath
{
	std::string path;
};


struct MeshData
{
	std::string path;
	entt::hashed_string path_hashed;
	uint32_t mesh_id = 0;
	bool VAO_loaded = false;
	bool bad_load = false;
	bool started_load = false;
	bool destroy = false;
	unsigned int stride = 0;

	glm::mat4 offset_matrix = glm::mat4(1);

	GLuint base_vertex = 0;
	GLuint VAO = 0;
	std::vector<GLuint> VBOs;
	GLuint EBO = 0;

	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec3> tangets;
	std::vector<glm::vec3> bitangents;
	std::vector<std::vector<uint8_t> >colors_dimensions;
	std::vector<std::vector<glm::vec4>> colors;
	std::vector<std::vector<uint8_t> >uvs_dimensions;
	std::vector<std::vector<glm::vec3>> uvs;
	std::vector<GLuint> indices;
};