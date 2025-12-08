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
	FailedUpload,
	MarkDestroyed,
	Destroying,
	Destroyed
};

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
struct PreRenderRequest
{
	//hashed_string_64 mesh_hashed_path = hashed_string_64("invalidhash");
	
	glm::mat4 model_matrix = glm::mat4(1); 
	uint64_t mesh_hash;
	uint64_t tex_hash;
	//hashed_string_64 texture_hashed_path = hashed_string_64("invalidhash");
};

struct VAOLoadInfo
{
	GLuint vao;
	GLuint ebo;
	bool vao_loaded;
	std::vector<GLuint> VBOs;
	hashed_string_64 hashed_path;
};


struct LoadRequest
{
	std::string path;
};


struct RenderRequest
{
	//hashed_string_64 mesh_hashed_path; // change to just the hash
	glm::mat4 model_matrix = glm::mat4(0);
	uint64_t mesh_hash;
	uint64_t tex_hash;
	GLuint vao = 0;
	GLsizei indices_size = 0;
	GLuint shader_program = 0;
	uint32_t material_id = 0; // I don't know
	//hashed_string_64 tex_hashed_path;
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
	glm::mat4 view_matrix = glm::mat4(1);
	glm::mat4 proj_matrix = glm::mat4(1);
	hashed_string_64 frame_buffer_hashed;
	uint32_t priority = 5000;
	bool clear_color_buffer = true;
	bool clear_depth_buffer = true;
};

struct MeshDataContents
{

	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec3> tangets;
	std::vector<glm::vec3> bitangents;
	std::vector<std::vector<uint8_t> >colors_dimensions;
	std::vector<std::vector<glm::vec4>> colors;
	std::vector<std::vector<uint8_t> >uvs_dimensions;
	std::vector<std::vector<glm::vec3>> uvs;

	std::vector<GLuint> indices;
	std::vector<GLuint> VBOs;
	GLuint EBO = 0;

	GLuint VAO = 0;

	ELoadStatus ram_load_status = ELoadStatus::NotLoaded;
	ELoadStatus vao_load_status = ELoadStatus::NotLoaded;
};

struct MeshData
{
	MeshData() : contents(std::make_shared<MeshDataContents>()) { }
	//std::string path;
	hashed_string_64 path_hashed;
	//uint64_t test;
	//std::shared_ptr<MeshDataContents> mesh_data;
	
	std::shared_ptr<MeshDataContents> contents;

	//glm::mat4 offset_matrix = glm::mat4(1);

	//GLuint base_vertex = 0;
	//bool is_copy = false;
	//bool freed_mesh_data = false;
	//MeshDataContents* mesh_data = new MeshDataContents();
	//
	//~MeshData()
	//{
	//	if (!freed_mesh_data && !is_copy)
	//		delete mesh_data;
	//}

	//MeshData(const MeshData& other) // for copying
	//	: path_hashed(other.path_hashed),
	//	VAO(other.VAO),
	//	ram_load_status(other.ram_load_status),
	//	vao_load_status(other.vao_load_status),
	//	is_copy(true),
	//	freed_mesh_data(other.freed_mesh_data),
	//	mesh_data(new MeshDataContents(*other.mesh_data)) {}
};

struct TextureGenInfo
{
	hashed_string_64 hashed_path;
	GLuint texture_id;
	ELoadStatus texture_gen_status;
};

struct TextureDataInfo
{
	float border_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	uint64_t tex_hash;
	GLsizei width = 0;
	GLsizei height = 0;

	GLenum color_format = GL_RGB;
	GLenum wrap_x = GL_REPEAT;
	GLenum wrap_y = GL_REPEAT;
	GLenum filtering_min = GL_NEAREST;
	GLenum filtering_mag = GL_LINEAR;
	GLenum mipmap_filtering = GL_LINEAR_MIPMAP_LINEAR;
	bool generate_mipmap = true;
};

struct TextureData
{
	TextureData() : texture_data_info(std::make_shared<TextureDataInfo>()) {}
	hashed_string_64 hashed_path;
	//uint64_t test;
	std::shared_ptr<TextureDataInfo> texture_data_info;
	std::shared_ptr<stbi_uc> texture_data;
	//std::string path = " ";



	//bool freed_texture_data_info = true;
	//TextureDataInfo* texture_data_info;
	//
	//bool freed_texture_data = true;
	//stbi_uc* stbi_texture_data;
	//

	ELoadStatus texture_ram_status = ELoadStatus::NotLoaded;
	ELoadStatus texture_gen_status = ELoadStatus::NotLoaded;

	GLuint texture_id = 0;
	uint8_t bound_index = 0;
	

	//~TextureData()
	//{
	//	//if (!freed_texture_data)
	//	//	stbi_image_free(stbi_texture_data);
	//	//if (!freed_texture_data_info)
	//	//	delete stbi_texture_data;
	//}
};

struct VAOLoadRequest
{
	MeshData mesh_data;
};
struct TexGenRequest
{
	TextureData texture_data;
};
