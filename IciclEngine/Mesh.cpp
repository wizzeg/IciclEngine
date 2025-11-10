#pragma once
#include "Mesh.h"
#include <numeric>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <print>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#ifndef ASSIMP_LOAD_FLAGS
#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices)
#endif


Mesh::Mesh() : initialized(false), hasMeshData(false) { std::cout << "Mesh Empty!" << std::endl; }

Mesh::Mesh(const char* path)
{
	if (vertexAttributeNums != nullptr)
	{
		delete vertexAttributeNums;
	}


	Assimp::Importer importer;

	const aiScene* pScene = importer.ReadFile(path, ASSIMP_LOAD_FLAGS);
	bool ret = false;
	//vertexAttributeNums = new VertexAttributeNums[pScene->mNumMeshes];

	vertexAttributes.clear();

	if (pScene)
	{
		int numberVerts = 0;
		vertexAttributes.reserve(pScene->mNumMeshes);
		vertexAttributeNums = new VertexAttributeNums[pScene->mNumMeshes];
		if (pScene->HasMeshes())
		{
			//std::println("Has Mesh");
			for (size_t i = 0; i < pScene->mNumMeshes; i++)
			{

				//std::println("Mesh {} has Positons: {}, UVs: {}, Colors: {}", i, pScene->mMeshes[i]->HasPositions(), pScene->mMeshes[i]->HasTextureCoords(0), pScene->mMeshes[i]->HasVertexColors(0));
				//std::println("tangets stuff {}", pScene->mMeshes[i]->HasTangentsAndBitangents());
				if (pScene->mMeshes[i]->HasPositions())
				{
					vertexAttributeNums[i].Positions = pScene->mMeshes[i]->mNumVertices;
					numberVerts += vertexAttributeNums[i].Positions;
				}
				
				for (size_t j = 0; j < 8; j++)
				{
					if (pScene->mMeshes[i]->HasTextureCoords(j))
					{
						bool CoordsAre3D = false;
						vertexAttributeNums[i].TextureCoords++;
						aiVector3f* temp;
						//
						vertexAttributeNums[i].TextureCoordsDimensions.push_back(pScene->mMeshes[i]->mNumUVComponents[j]);
					}
				}

				for (size_t j = 0; j < 8; j++)
				{
					if (pScene->mMeshes[i]->HasVertexColors(j))
					{
						vertexAttributeNums[i].VertexColors++;
					}
				}

				vertexAttributeNums[i].Normals = pScene->mMeshes[i]->HasNormals();
				vertexAttributeNums[i].TangentsBitangents = pScene->mMeshes[i]->HasTangentsAndBitangents();
				//std::println("Has tangets {}", vertexAttributeNums[i].TangentsBitangents);
				vertexAttributeNums[i].MaterialIndex = pScene->mMeshes[i]->mMaterialIndex;

				vertexAttributes.push_back(new VertexAttributes(&vertexAttributeNums[i]));

				aiVector3D* temp = pScene->mMeshes[i]->mVertices;
				for (size_t j = 0; j < pScene->mMeshes[i]->mNumVertices; j++)
				{
					vertexAttributes[i]->Positions.push_back(glm::vec3(temp[j].x, temp[j].y, temp[j].z));
				}
				if (vertexAttributeNums[i].Normals == true)
				{
					temp = pScene->mMeshes[i]->mNormals;
					for (size_t j = 0; j < pScene->mMeshes[i]->mNumVertices; j++)
					{
						vertexAttributes[i]->Normals.push_back(glm::vec3(temp[j].x, temp[j].y, temp[j].z));
					}

				}

				aiVector3D* tempTangents;
				tempTangents = pScene->mMeshes[i]->mTangents;
				if (vertexAttributeNums[i].TangentsBitangents == true)
				{
					for (size_t j = 0; j < pScene->mMeshes[i]->mTangents->Length(); j++)
					{
						vertexAttributes[i]->Tangents.push_back(glm::vec3(tempTangents[j].x, tempTangents[j].y, tempTangents[j].z));
					}

					tempTangents = pScene->mMeshes[i]->mBitangents;
					for (size_t j = 0; j < pScene->mMeshes[i]->mBitangents->Length(); j++)
					{
						vertexAttributes[i]->Bitangets.push_back(glm::vec3(tempTangents[j].x, tempTangents[j].y, tempTangents[j].z));
					}
				}


				aiVector3D* temp2;
				for (size_t j = 0; j < vertexAttributeNums[i].TextureCoords; j++)
				{
					temp2 = pScene->mMeshes[i]->mTextureCoords[j];
					for (size_t k = 0; k < pScene->mMeshes[i]->mNumVertices; k++)
					{
						vertexAttributes[i]->UVs[j].push_back(glm::vec3(temp2[k].x, temp2[k].y, temp2[k].z));
					}
				}

				aiColor4D* temp3;
				for (size_t j = 0; j < vertexAttributeNums[i].VertexColors; j++)
				{
					temp3 = pScene->mMeshes[i]->mColors[j];
					for (size_t k = 0; k < pScene->mMeshes[i]->mNumVertices; k++)
					{
						vertexAttributes[i]->Colors[j].push_back(glm::vec4(temp3[k].r, temp3[k].g, temp3[k].b, temp3[k].a));
					}
				}


				std::vector<float>& buffer = vertexAttributes[i]->buffer;
				for (size_t j = 0; j < vertexAttributeNums[i].Positions; j++)
				{

					buffer.push_back(vertexAttributes[i]->Positions[j].x);
					buffer.push_back(vertexAttributes[i]->Positions[j].y);
					buffer.push_back(vertexAttributes[i]->Positions[j].z);


					if (vertexAttributeNums[i].Normals > 0)
					{
						buffer.push_back(vertexAttributes[i]->Normals[j].x);
						buffer.push_back(vertexAttributes[i]->Normals[j].y);
						buffer.push_back(vertexAttributes[i]->Normals[j].z);
					}

					for (size_t l = 0; (l < vertexAttributeNums[i].VertexColors) && l < MAXUVCOLORCHANNELS; l++)
					{
						buffer.push_back(vertexAttributes[i]->Colors[l][j].x);
						buffer.push_back(vertexAttributes[i]->Colors[l][j].y);
						buffer.push_back(vertexAttributes[i]->Colors[l][j].z);
						buffer.push_back(vertexAttributes[i]->Colors[l][j].w);
					}

					for (size_t l = 0; (l < vertexAttributeNums[i].TextureCoords) && l < MAXUVCOLORCHANNELS; l++)
					{
						if (vertexAttributeNums[i].TextureCoordsDimensions[l] >= 1)
						{
							buffer.push_back(vertexAttributes[i]->UVs[l][j].x);
						}
						if (vertexAttributeNums[i].TextureCoordsDimensions[l] >= 2)
						{
							buffer.push_back(vertexAttributes[i]->UVs[l][j].y);
						}
						if (vertexAttributeNums[i].TextureCoordsDimensions[l] >= 3)
						{
							buffer.push_back(vertexAttributes[i]->UVs[l][j].z);
						}
					}
				}
				
				for (size_t k = 0; k < pScene->mMeshes[i]->mNumFaces; k++)
				{
					const aiFace& face = pScene->mMeshes[i]->mFaces[k];
					vertexAttributes[i]->indices.reserve(face.mNumIndices);
					for (unsigned int l = 0; l < face.mNumIndices; l++)
					{
						vertexAttributes[i]->indices.push_back(face.mIndices[l]);
					}
				}

			}
		}
		BufferMesh();
		std::println("Mesh has submeshes {}, and total number of vertices {}", pScene->mNumMeshes, numberVerts);
	}
}

Mesh::Mesh(std::vector<unsigned int>&& indices, std::vector<float>&& vertices, std::vector<unsigned int>&& offsets) 
	: indices(std::move(indices)), vertices(std::move(vertices)), offsets(std::move(offsets)), hasMeshData(true)
{
	//std::cout << "Mesh Moved!" << std::endl;
	InitializeBuffers();
}
Mesh::Mesh(const std::vector<unsigned int>& indices, const std::vector<float>& vertices, const std::vector<unsigned int>& offsets)
	: indices(indices), vertices(vertices), offsets(offsets), hasMeshData(true)
{
	//std::cout << "Mesh Copied!" << std::endl;
	InitializeBuffers();
}

void Mesh::InitializeBuffers()
{
	if (!hasMeshData) return;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);					// this is our raw data??? (I think is sets up a binder, and it basically stores all the things below)

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);		// the previous is my reference
	glBufferData(GL_ARRAY_BUFFER, (vertices.size() * sizeof(float)), (void*)vertices.data(), GL_STATIC_DRAW); // load vertices

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (indices.size() * sizeof(unsigned int)), (void*)indices.data(), GL_STATIC_DRAW);

	float stride = std::accumulate(offsets.begin(), offsets.end(), 0.0f);
	unsigned int offset = 0;
	for (unsigned int i = 0; i < offsets.size(); i++)
	{
		glVertexAttribPointer(i, offsets[i], GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(offset * sizeof(float)));
		glEnableVertexAttribArray(i);
		offset += offsets[i];
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	initialized = true;
}

void Mesh::Render() {
	if (initialized || hasMeshData)
	{
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		std::println("drawing with wrong");
		//glBindVertexArray(0);
	}
	else if (vertexAttributeNums != nullptr && !vertexAttributes.empty())
	{
		for (size_t i = 0; i < vertexAttributes.size(); i++)
		{
			glBindVertexArray(vertexAttributes[i]->VAO);
			glDrawElements(GL_TRIANGLES, vertexAttributes[i]->indices.size(), GL_UNSIGNED_INT, 0);
			//PROBLEM HERE... THIS IS WEIRD...
			//break;
		}
	}
}

Mesh::~Mesh() {
	if (initialized)
	{
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
	}
}

void Mesh::BufferMesh()
{
	if (vertexAttributeNums == nullptr || vertexAttributes.size() <= 0)
	{
		std::println("No mesh has been initialized");
		return;
	}

	for (size_t i = 0; i < vertexAttributes.size(); i++)
	{
		if (vertexAttributes[i]->VBO != 0)
		{
			glBindBuffer(GL_ARRAY_BUFFER, vertexAttributes[i]->VBO);
			glDeleteBuffers(1, &vertexAttributes[i]->VBO);
		}
		if (vertexAttributes[i]->VAO != 0)
		{
			glBindVertexArray(0);
			glDeleteVertexArrays(1, &vertexAttributes[i]->VAO);
		}

		glGenVertexArrays(1, &vertexAttributes[i]->VAO);
		glBindVertexArray(vertexAttributes[i]->VAO);

		glGenBuffers(1, &vertexAttributes[i]->EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexAttributes[i]->EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (vertexAttributes[i]->indices.size() * sizeof(unsigned int)), (void*)vertexAttributes[i]->indices.data(), GL_STATIC_DRAW);

		glGenBuffers(1, &vertexAttributes[i]->VBO);
		glBindBuffer(GL_ARRAY_BUFFER, vertexAttributes[i]->VBO);
		glBufferData(GL_ARRAY_BUFFER, (vertexAttributes[i]->buffer.size() *  sizeof(float)), (void*)vertexAttributes[i]->buffer.data(), GL_STATIC_DRAW); // load vertices

		unsigned int offset = 0;

		// Locations
		//Position = 0
		//Normals = 1
		//Tangets = 2
		//Bitangets = 3
		//Colors = 4 to 9
		//UVs = 10 to 15

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexAttributes[i]->stride * sizeof(float), (void*)(offset * sizeof(float)));
		glEnableVertexAttribArray(0);
		offset += 3;

		if (vertexAttributes[i]->Normals.size() > 0)
		{
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexAttributes[i]->stride * sizeof(float), (void*)(offset * sizeof(float)));
			glEnableVertexAttribArray(1);
			offset += 3;
		}

		if (vertexAttributes[i]->Tangents.size() > 0)
		{
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, vertexAttributes[i]->stride * sizeof(float), (void*)(offset * sizeof(float)));
			glEnableVertexAttribArray(2);
			offset += 3;
		}
		if (vertexAttributes[i]->Bitangets.size() > 0)
		{
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, vertexAttributes[i]->stride * sizeof(float), (void*)(offset * sizeof(float)));
			glEnableVertexAttribArray(3);
			offset += 3;
		}
		unsigned int location = 4;
		for (unsigned int j = 0; j < vertexAttributeNums[i].VertexColors && j < MAXUVCOLORCHANNELS; j++)
		{
			if (vertexAttributes[i]->Colors[j].size() > 0)
			{
				glVertexAttribPointer(location + j, 4, GL_FLOAT, GL_FALSE, vertexAttributes[i]->stride * sizeof(float), (void*)(offset * sizeof(float)));
				glEnableVertexAttribArray(location + j);
				offset += 4;
			}
		}
		location = 10;
		for (unsigned int j = 0; j < vertexAttributeNums[i].VertexColors && j < MAXUVCOLORCHANNELS; j++)
		{
			if (vertexAttributes[i]->Colors[j].size() > 0)
			{
				glVertexAttribPointer(location + j, vertexAttributeNums[i].TextureCoordsDimensions[j], GL_FLOAT, GL_FALSE, vertexAttributes[i]->stride * sizeof(float), (void*)(offset * sizeof(float)));
				glEnableVertexAttribArray(location + j);
				offset += vertexAttributeNums[i].TextureCoordsDimensions[j];
			}
		}
	}
	glBindVertexArray(0);
}