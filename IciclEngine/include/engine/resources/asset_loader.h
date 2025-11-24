#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>

#include <engine/renderer/render_info.h>
#include <engine/resources/obj_parser.h>

#ifndef ASSIMP_LOAD_FLAGS
#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices)
#endif

struct AssetLoader
{
	AssetLoader() {};
	~AssetLoader() {};

	std::uint32_t load_from_path(const std::string& a_path);
private:

	Assimp::Importer importer;
	ObjParser obj_importer;

	//size_t iterate_node(aiNode* a_node, aiMesh* a_meshes[], size_t a_index, glm::mat4 a_transform, std::vector<VertexAttributeNums> a_attribute_nums, std::vector<VertexData> a_vertexData);
	//VertexData load_sub_mesh(VertexAttributeNums& a_attribute_nums, glm::mat4 a_transform, aiMesh* a_mesh);
	//void count_attribute_nums(VertexAttributeNums& a_attribute_nums, const aiMesh* a_mesh);
};

