#pragma once
#include <string>
#include <vector>
#include <engine/utilities/hashed_string_64.h>
#include <glm/glm.hpp>
#include <glad/glad.h>
#ifndef MAX_UVS_COLORS
#define MAX_UVS_COLORS 6;
#endif
#include <stb_image/stb_image.h>
#include <memory>

struct PreRenderRequest
{
	hashed_string_64 hashed_path;
	glm::mat4 model_matrix;
};

struct VAOLoadInfo
{
	hashed_string_64 hashed_path;
	bool vao_loaded;
	GLuint vao;
	std::vector<GLuint> VBOs;
	GLuint ebo;
};

struct LoadRequest
{
	std::string path;
};


struct RenderRequest
{
	hashed_string_64 hashed_path;
	GLuint vao = 0;
	GLsizei indices_size = 0;
	glm::mat4 model_matrix = glm::mat4(0);
	GLuint shader_program = 0;
	uint32_t material_id = 0; // I don't know

};

enum BufferAttributeLocation : uint8_t
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

struct CameraData
{
	bool clear_color_buffer = true;
	bool clear_depth_buffer = true;
	uint32_t priority;
	glm::mat4 view_matrix;
	glm::mat4 proj_matrix;
	hashed_string_64 frame_buffer_hashed;
};

enum ELoadStatus : uint8_t
{
	NotLoaded,
	RequestedLoad,
	StartedLoad,
	Loaded,
	FailedLoadBadPath,
	FailedLoadNoAccess,
	FailedLoadOpen,
	FailedLoadBadModel,
	FailedLoadNoSpace,
	FailedUpload
};


struct MeshData
{
	std::string path;
	hashed_string_64 path_hashed;
	uint32_t mesh_id = 0;
	bool VAO_loaded = false;
	bool bad_load = false;
	bool started_load = false;
	bool destroy = false;
	unsigned int stride = 0;

	ELoadStatus ram_load_status = ELoadStatus::NotLoaded;
	ELoadStatus vao_load_status = ELoadStatus::NotLoaded;

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
struct VAOLoadRequest
{
	MeshData mesh_data;
};

struct TextureGenInfo
{
	hashed_string_64 hashed_path;
	GLuint texture_id;
	ELoadStatus texture_gen_status;
};

struct TextureData
{
	hashed_string_64 hashed_path;
	std::string path = " ";

	GLuint texture_id = 0;
	uint8_t bound_index = 0;
	GLsizei width = 0;
	GLsizei height = 0;

	float border_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	GLenum color_format = GL_RGB;
	GLenum wrap_x = GL_REPEAT;
	GLenum wrap_y = GL_REPEAT;
	GLenum filtering_min = GL_NEAREST;
	GLenum filtering_mag = GL_LINEAR;
	bool generate_mipmap = true;
	GLenum mipmap_filtering = GL_LINEAR_MIPMAP_LINEAR;
	std::shared_ptr<stbi_uc> texture_data;

	ELoadStatus texture_ram_status = ELoadStatus::NotLoaded;
	ELoadStatus texture_gen_status = ELoadStatus::NotLoaded;
};