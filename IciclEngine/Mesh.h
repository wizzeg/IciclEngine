#pragma once

#include <vector>
#include "glm/glm.hpp"
#ifndef MAXUVCOLORCHANNELS
#define MAXUVCOLORCHANNELS 6
#endif

struct VertexAttributeNums
{
public:
	VertexAttributeNums() : Positions(0), TextureCoords(0), VertexColors(0), MaterialIndex(0), TangentsBitangents(false), Normals(false){};
	unsigned int Positions;
	unsigned int TextureCoords;
	std::vector<unsigned int> TextureCoordsDimensions;
	unsigned int VertexColors;
	unsigned int MaterialIndex;
	bool TangentsBitangents;
	bool Normals;
};

struct VertexAttributes
{
public:
	bool initialized = false;
	unsigned int stride = 0;

	unsigned int VAO = 0;
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
	VertexAttributes()
	{
	}
	VertexAttributes(VertexAttributeNums* vertexAttributeNums)
	{
		unsigned int reserveSize = vertexAttributeNums->Positions;
		stride = 0;
		Positions.reserve(reserveSize);
		stride += 3;
		if (vertexAttributeNums->Normals > 0)
		{
			Normals.reserve(reserveSize);
			stride += 3;
		}
		if (vertexAttributeNums->TangentsBitangents > 0)
		{
			Tangents.reserve(reserveSize);
			stride += 3;
		}

		if (vertexAttributeNums->TangentsBitangents > 0)
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

struct Textures
{

};


class Mesh
{
	unsigned int VBO = 0;
	unsigned int VAO = 0;
	unsigned int EBO = 0;
	bool initialized = false;
	bool hasMeshData = false;

	VertexAttributeNums* vertexAttributeNums = nullptr;
	std::vector<VertexAttributes*> vertexAttributes;
	std::vector<unsigned int> indicies;


	std::vector<unsigned int> indices;
	std::vector<float> vertices;
	std::vector<unsigned int> offsets;


	//std::vector<> positions;
	//std::vector<>


public:
	void Render();
	void InitializeBuffers();
	void BufferMesh();

	Mesh();
	Mesh(const char* path);
	Mesh(std::vector<unsigned int>&& indicies, std::vector<float>&& verticies, std::vector<unsigned int>&& offsets);
	Mesh(const std::vector<unsigned int>& indicies, const std::vector<float>& verticies, const std::vector<unsigned int>& offsets);
	~Mesh();
};