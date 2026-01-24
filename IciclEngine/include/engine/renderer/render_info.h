#pragma once
#include <string>
#include <vector>
#include <engine/utilities/hashed_string_64.h>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <glm/mat4x4.hpp>
#ifndef MAX_UVS_COLORS
#define MAX_UVS_COLORS 6;
#endif
#include <stb_image/stb_image.h>
#include <memory>
#include <engine/resources/data_structs.h>
#include <variant>

//using UniformValue = std::variant<bool, int, float, double, glm::vec3, glm::vec4, glm::quat, glm::mat4, glm::ivec1, std::string>;

namespace UniformTypes
{
	enum Type : uint8_t
	{
		vec1i,
		vec1ui,
		vec3f,
		vec4f,
		vec2f,
		mat4f,
		mat3f // I think I can add e.g. UBO mat4f later.
	};
}

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
	Destroyed,
	ShaderLoadedPath,
	ShaderLoadedProgram,
	ShaderProgramRequested,
	MaterialDependenciesLoading,
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
	glm::mat4 model_matrix = glm::mat4(1); 
	uint64_t mesh_hash;
	uint64_t tex_hash;
};

struct VAOLoadInfo
{
	hashed_string_64 hashed_path;
	GLuint vao;
	GLuint ebo;
	uint64_t modified_time = 0;
	std::vector<GLuint> VBOs;
};


struct LoadRequest
{
	std::string path;
};


struct RenderRequest
{
	glm::mat4 model_matrix = glm::mat4(0);
	uint64_t mesh_hash;
	uint64_t tex_hash;
	GLuint vao = 0;
	GLsizei indices_size = 0;
	GLuint shader_program = 0;
	uint32_t material_id = 0; // I don't know
};

struct PreRenderReq
{
	glm::mat4 model_matrix = glm::mat4(1);
	uint64_t mesh_hash;
	uint64_t mat_hash;
	bool instanced;
	bool mipmap;
};

struct RenderReq
{
	glm::mat4 model_matrix = glm::mat4(0);
	uint64_t mesh_hash;
	uint64_t mat_hash;
	GLuint vao = 0;
	GLsizei indices_size = 0;
	bool instanced;
	bool mipmap;
	//uint16_t material_version;
	GLuint gl_program;
	//std::vector<UniformData> uniforms;
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
	glm::vec3 position;
};

struct MeshDataContents
{
	hashed_string_64 hashed_path;
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec4> tangents;
	std::vector<glm::vec3> bitangents;
	std::vector<std::vector<uint8_t> >colors_dimensions;
	std::vector<std::vector<glm::vec4>> colors;
	std::vector<std::vector<uint8_t> >uvs_dimensions;
	std::vector<std::vector<glm::vec3>> uvs;
	std::vector<GLuint> indices;
	
	std::vector<GLuint> VBOs;
	GLuint EBO = 0;
};

struct MeshData
{
	MeshData() : contents(std::make_shared<MeshDataContents>()) { }
	uint64_t hash = 14695981039346656037; // hash of ""
	GLuint VAO = 0;
	GLsizei num_indicies = 0;
	std::shared_ptr<MeshDataContents> contents;
	uint64_t modified_time = 0;
	ELoadStatus ram_load_status = ELoadStatus::NotLoaded;
	ELoadStatus vao_load_status = ELoadStatus::NotLoaded;
	ELoadStatus runtime_gen = ELoadStatus::NotLoaded;
};

struct TextureGenInfo
{
	hashed_string_64 hashed_path;
	GLuint texture_id = 0;
	ELoadStatus texture_gen_status = ELoadStatus::NotLoaded;
	uint64_t modified_time = 0;
};

struct ProgramLoadInfo
{
	hashed_string_64 hashed_path;
	GLint gl_program = 0;
	ELoadStatus program_load_status = ELoadStatus::NotLoaded;
	uint64_t job_time;
};

enum EDependecyType : uint8_t
{
	All,
	Texture,
	Shader
};

struct ValidateMatDependencies
{
	hashed_string_64 hashed_path;
	EDependecyType dependency_check;
	bool unloaded;
	uint64_t job_time;
};

struct TextureDataContents
{
	hashed_string_64 hashed_path;
	float border_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	GLsizei width = 0;
	GLsizei height = 0;
	GLenum color_format = GL_RGB;
	GLenum wrap_x = GL_REPEAT;
	GLenum wrap_y = GL_REPEAT;
	GLenum filtering_min = GL_LINEAR;
	GLenum filtering_mag = GL_LINEAR;
	GLenum mipmap_filtering = GL_LINEAR_MIPMAP_LINEAR;
	std::shared_ptr<stbi_uc> texture_data;
	uint16_t num_references = 0;
	bool generate_mipmap = true;
};

struct TextureData // needs to store time aswell, to avoid race conditions
{
	TextureData() : contents(std::make_shared<TextureDataContents>()) {}
	uint64_t hash = 14695981039346656037; // hash of ""
	GLuint texture_id = 0;
	uint8_t bound_index = 0;
	ELoadStatus texture_ram_status = ELoadStatus::NotLoaded;
	ELoadStatus texture_gen_status = ELoadStatus::NotLoaded;
	std::shared_ptr<TextureDataContents> contents;
	uint64_t modified_time = 0;
	uint32_t num_references = 0;
};

struct TextureMiniData
{
	GLuint texture_id;
	uint8_t bound_index;
};

struct VAOLoadRequest
{
	MeshData mesh_data;
};


struct TexGenRequest
{
	TextureData texture_data;
};

struct ShaderData
{
	hashed_string_64 hashed_path = "";
	std::string frag_path;
	std::string vert_path;
	std::string frag_buffer;
	std::string vert_buffer;
	GLuint gl_program = 0;
	ELoadStatus loading_status = ELoadStatus::NotLoaded;
	uint64_t modified_time = 0;
	std::string name;
	uint32_t num_references = 0;
};

struct ProgramLoadRequest
{
	ShaderData shader_data;
};

struct UniformMeta
{
	hashed_string_64 name_location;
	UniformTypes::Type type;
	uint32_t count;
	GLint location;
};

struct TextureMeta
{
	hashed_string_64 name_location;
	uint8_t index;
	GLenum target;
};

struct ShaderTemplate
{
	hashed_string_64 hashed_path;
	std::string vertex_path = "vertex path";
	std::string fragment_path = "fragment path";
	GLuint gl_program;

	std::vector<UniformMeta> uniform_slots; // sort by hash -> used only to validate material? Do it only check on material load?
	std::vector<TextureMeta> texture_slots; // sort by index. -> verify material binds at right place...
};

struct MaterialUniform
{
	UniformMeta meta;
	UniformValue data;
};

struct MaterialTexture
{
	TextureMeta meta;
	hashed_string_64 hashed_path;
	GLuint texture_id;
};

struct RuntimeUniform
{
	UniformValue value = "";
	std::string location = "";
	std::type_index type = typeid(std::string);
	GLint texture_id = 0;
};

struct RuntimeTexture
{
	GLuint bind_index;
	GLuint texture_id;
};

struct TexDependency
{
	uint64_t hash;
	bool fulfilled = false;
};

struct MaterialData // this is fine to keep large...
{
	hashed_string_64 hashed_path = hashed_string_64("");
	hashed_string_64 shader_path = hashed_string_64("");
	//hashed_string_64 hashed_shader_path;
	
	std::vector<UniformData> uniforms;
	std::vector<TexDependency> tex_deps;
	uint64_t modified_time = 0;
	GLuint gl_program = 0; // get from shader
	uint64_t program_modified_time = 0;
	bool is_lit = false;
	bool recieves_shadows = false;
	bool casts_shadows = false;
	bool instanced = false;
	bool transparent = false;
	bool is_deffered = false;
	bool added_shader_reference = false;
	ELoadStatus load_status = ELoadStatus::NotLoaded;
	std::string name = "";
};
//This is the path that's entered... this must then load the shader

struct RuntimeMaterial // pushed to the Render thread, similar to vao load, but it now stores data aswell
{
	//uint64_t material_id;
	uint64_t hash;
	GLuint gl_program = 0;
	std::vector<RuntimeUniform> uniforms;
	//std::vector<RuntimeTexture> textures;
	bool is_lit;
	bool recieves_shadows;
	bool casts_shadows;
	bool instantiable;
	bool transparent;
	bool is_deffered;
};

struct RuntimeMesh
{
	uint64_t hash;
	GLsizei num_indices;
	GLuint vao;
};

struct RuntimePrerenderRequest
{
	glm::mat4 model_matrix;
	uint64_t mesh_hash;
	uint64_t material_hash;
};

struct RuntimeRenderCall
{
	glm::mat4 model_matrix;
	uint64_t material_hash;
	GLuint vao;
	GLsizei index_count;
};

struct RenderContext
{
	std::vector<RenderReq> render_requests;
	std::vector<RuntimeMaterial> materials;
	
	std::vector<PointLightSSBO> lights;
	std::vector<ShadowLight> shadow_lights;
};