#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>

#ifndef MAX_UVS_COLORS
#define MAX_UVS_COLORS 6;
#endif

enum BufferType
{
	POSITION_VB,
	NORMAL_VB,
	TANGENT_VB,
	BITANGET_VB,
	COLOR0_VB,
	COLOR1_VB,
	COLOR2_VB,
	COLOR3_VB,
	COLOR4_VB,
	COLOR5_VB,
	UVCoords0_VB,
	UVCoords1_VB,
	UVCoords2_VB,
	UVCoords3_VB,
	UVCoords4_VB,
	UVCoords5_VB,
};

struct MeshHandle
{
	std::uint32_t id;
};

struct MaterialHandle
{
	std::uint32_t id;
};

struct Asset3D
{
	std::string path;
};



struct VertexAttributeNums
{
	VertexAttributeNums() : Positions(0), TextureCoords(0), VertexColors(0), MaterialIndex(0), TangentsBitangents(false), Normals(false) {};
	unsigned int Positions;
	unsigned int TextureCoords;
	std::vector<unsigned int> TextureCoordsDimensions;
	unsigned int VertexColors;
	unsigned int MaterialIndex;
	bool TangentsBitangents;
	bool Normals;
};

struct VertexData
{
	bool initialized = false;
	unsigned int stride = 0;

	size_t base_vertex = 0;

	unsigned int VAO = 0; // don't think I'll use these 3 ... 
	unsigned int VBO = 0;
	unsigned int EBO = 0;



	std::vector<glm::vec3> Positions;
	std::vector<glm::vec3> Normals;
	std::vector<glm::vec3> Tangents;
	std::vector<glm::vec3> Bitangets;
	std::vector<std::vector<glm::vec4>> Colors;
	std::vector<std::vector<glm::vec3>> UVs;
	std::vector<float> buffer;
	std::vector<unsigned int> indices;
	VertexData()
	{
	}
	VertexData(VertexAttributeNums* vertexAttributeNums)
	{
		unsigned int reserveSize = vertexAttributeNums->Positions;
		stride = 0;
		Positions.reserve(reserveSize);
		stride += 3;
		if (vertexAttributeNums->Normals)
		{
			Normals.reserve(reserveSize);
			stride += 3;
		}
		if (vertexAttributeNums->TangentsBitangents)
		{
			Tangents.reserve(reserveSize);
			stride += 3;
		}

		if (vertexAttributeNums->TangentsBitangents)
		{
			Bitangets.reserve(reserveSize);
			stride += 3;
		}
		if (vertexAttributeNums->VertexColors > 0)
		{
			Colors.reserve(vertexAttributeNums->VertexColors);
			for (unsigned int i = 0; i < vertexAttributeNums->VertexColors; i++)
			{
				Colors.push_back(std::vector<glm::vec4>());
				Colors[i].reserve(reserveSize);
				stride += 4;
			}
		}
		if (vertexAttributeNums->TextureCoords > 0)
		{
			UVs.reserve(vertexAttributeNums->TextureCoords);
			for (unsigned int i = 0; i < vertexAttributeNums->TextureCoords; i++)
			{
				UVs.push_back(std::vector<glm::vec3>());
				UVs[i].reserve(reserveSize);
				stride += vertexAttributeNums->TextureCoordsDimensions[i];
			}
		}
		buffer.reserve(stride * reserveSize);
		initialized = true;
	}
};

struct SubMesh
{
	glm::mat4 transform;
	VertexAttributeNums attribute_nums;
	VertexData vertex_data;
};

struct Mesh
{
	VertexAttributeNums max_attribute_nums; // fill in trash defaults 0 if not all submeshes have all attributes
	std::vector<SubMesh> sub_meshes;
};