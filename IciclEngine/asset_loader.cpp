#include "asset_loader.h"
#include "render_info.h"
#include "macros.h"
#include <vector>
#include "static_conversions.h"

std::uint32_t AssetLoader::load_from_path(const std::string& a_path)
{
	const aiScene* ai_scene = importer.ReadFile(a_path, ASSIMP_LOAD_FLAGS);
	if (ai_scene)
	{
		if (ai_scene->HasMeshes())
		{
			std::vector<VertexAttributeNums> vertex_attribute_nums;
			vertex_attribute_nums.reserve(ai_scene->mNumMeshes + 1);
			for (size_t i = 0; i < ai_scene->mNumMeshes + 1; i++)
			{
				vertex_attribute_nums.emplace_back(VertexAttributeNums{});
			}
			std::uint32_t total_vertices = 0;

			std::vector<VertexData> vertex_datas;
			vertex_datas.reserve(ai_scene->mNumMeshes);

			PRINTLN("number of meshes: {}", ai_scene->mNumMeshes);

			auto root_node = ai_scene->mRootNode;
			for (size_t i = 0; i < root_node->mNumMeshes; i++)
			{
				i++;
			}
		}
	}
	return 0;
}

size_t AssetLoader::iterate_node(aiNode* a_node, aiMesh* a_meshes[], size_t a_index, glm::mat4 a_transform, std::vector<VertexAttributeNums> a_attribute_nums, std::vector<VertexData> a_vertexData)
{
	auto num_meshes = a_node->mNumMeshes;
	auto transform = conversions::ai_matrix4x4_to_glm_mat4(a_node->mTransformation);
	for (; a_index < num_meshes; a_index++)
	{
		aiMesh* ai_mesh = a_meshes[a_node->mMeshes[a_index]];
		count_attribute_nums(a_attribute_nums[a_index], ai_mesh);
		VertexData vertex_data = load_sub_mesh(a_attribute_nums[a_index], transform, ai_mesh);
		a_vertexData.push_back(vertex_data);
	}
	for (size_t i = 0; i < a_node->mNumChildren; i++)
	{
		a_index = iterate_node(a_node->mChildren[i], a_meshes, a_index, transform, a_attribute_nums, a_vertexData);
	}
	return a_index;
	// this will look at all the meshes, and then launch load_sub_mesh with handles for the data/attributes, and their current transform.
}

VertexData AssetLoader::load_sub_mesh(VertexAttributeNums& a_attribute_nums, glm::mat4 a_transform, aiMesh* a_mesh)
{
	//count_attribute_nums(a_attribute_nums, a_mesh);
	return VertexData/* vertex_data*/(&a_attribute_nums);
	
	// fill vertex attribute nums, and initialize vertexData, calculate the transform.
}

void AssetLoader::count_attribute_nums(VertexAttributeNums& a_attribute_nums, const aiMesh* a_mesh)
{
	if (a_mesh->HasPositions())
	{
		a_attribute_nums.Positions = a_mesh->mNumVertices;
	}

	for (size_t j = 0; j < 6; j++) // MAX_UVS_COLORS apparently can't be used here...
	{
		if (a_mesh->HasTextureCoords(j))
		{
			a_attribute_nums.TextureCoords++;
			a_attribute_nums.TextureCoordsDimensions.push_back(a_mesh->mNumUVComponents[j]);
		}
		else break;
	}

	for (size_t j = 0; j < 6; j++)
	{
		if (a_mesh->HasVertexColors(j))
		{
			a_attribute_nums.VertexColors++;
		}
		else break;
	}

	a_attribute_nums.Normals = a_mesh->HasNormals();
	a_attribute_nums.TangentsBitangents = a_mesh->HasTangentsAndBitangents();
	a_attribute_nums.MaterialIndex = a_mesh->mMaterialIndex;
}