#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <string>

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
	glm::uint pos, nrm, uv;
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

struct ObjMesh
{
	
	std::string path;
	std::vector<ObjPosition> verticies;
	std::vector<ObjColor> colors;
	std::vector<ObjNormal> normals;
	std::vector<ObjFace> faces;
	std::vector<ObjUVs> uvs;
};

struct ObjParser
{
	ObjMesh load_mesh_from_filepath(const std::string& a_path);
};

