#include <engine/renderer/vao_loader.h>
#include <engine/utilities/macros.h>
#include <glfw/glfw3.h>
#include <glad/glad.h>

bool VAOLoader::load_vao(MeshData& a_mesh)
{
	//if (a_mesh.bad_load)
	//{
	//	PRINTLN("mesh has had a bad load");
	//	return false;
	//}
	if (a_mesh.ram_load_status != ELoadStatus::Loaded)
	{
		// does not have mesh data
		a_mesh.ram_load_status = ELoadStatus::NotLoaded; 
		return false;
	}

	if (a_mesh.VAO != 0)
	{
		PRINTLN("VAO taken, clearing");
		for (size_t i = 0; i < a_mesh.contents->VBOs.size(); i++)
		{
			glDeleteBuffers(1, &a_mesh.contents->VBOs[i]);
		}
		a_mesh.contents->VBOs.clear();
		glDeleteBuffers(1, &a_mesh.contents->EBO);
		a_mesh.contents->EBO = 0;
		glDeleteBuffers(1, &a_mesh.VAO);
		a_mesh.VAO = 0;
		a_mesh.ram_load_status = ELoadStatus::NotLoaded;
		//a_mesh.contents->VAO_loaded = false;
	}
	PRINTLN("loading vao for mesh: {}", a_mesh.path_hashed.string);
	glGenVertexArrays(1, &a_mesh.VAO);
	glBindVertexArray(a_mesh.VAO);

	glGenBuffers(1, &a_mesh.contents->EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, a_mesh.contents->EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, SIZEOF_ELEMENTS_IN_VECTOR(a_mesh.contents->indices), a_mesh.contents->indices.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &a_mesh.contents->VBOs.emplace_back(0));
	glBindBuffer(GL_ARRAY_BUFFER, a_mesh.contents->VBOs[a_mesh.contents->VBOs.size() - 1]);
	glBufferData(GL_ARRAY_BUFFER, SIZEOF_ELEMENTS_IN_VECTOR(a_mesh.contents->positions), a_mesh.contents->positions.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(BufferAttributeLocation::Position, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(BufferAttributeLocation::Position);
	
	if (!a_mesh.contents->normals.empty())
	{
		glGenBuffers(1, &a_mesh.contents->VBOs.emplace_back(0));
		glBindBuffer(GL_ARRAY_BUFFER, a_mesh.contents->VBOs[a_mesh.contents->VBOs.size() - 1]);
		glBufferData(GL_ARRAY_BUFFER, SIZEOF_ELEMENTS_IN_VECTOR(a_mesh.contents->normals), a_mesh.contents->normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(BufferAttributeLocation::Normal, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(BufferAttributeLocation::Normal);

	}
	if (!a_mesh.contents->tangets.empty())
	{
		glGenBuffers(1, &a_mesh.contents->VBOs.emplace_back(0));
		glBindBuffer(GL_ARRAY_BUFFER, a_mesh.contents->VBOs[a_mesh.contents->VBOs.size() - 1]);
		glBufferData(GL_ARRAY_BUFFER, SIZEOF_ELEMENTS_IN_VECTOR(a_mesh.contents->tangets), a_mesh.contents->tangets.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(BufferAttributeLocation::Tangent, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(BufferAttributeLocation::Tangent);
	}
	if (!a_mesh.contents->bitangents.empty())
	{
		glGenBuffers(1, &a_mesh.contents->VBOs.emplace_back(0));
		glBindBuffer(GL_ARRAY_BUFFER, a_mesh.contents->VBOs[a_mesh.contents->VBOs.size() - 1]);
		glBufferData(GL_ARRAY_BUFFER, SIZEOF_ELEMENTS_IN_VECTOR(a_mesh.contents->bitangents), a_mesh.contents->bitangents.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(BufferAttributeLocation::Bitangent, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(BufferAttributeLocation::Bitangent);
	}

	size_t num_colors = 0;
	for (; num_colors < a_mesh.contents->colors.size(); num_colors++)
	{
		if (!a_mesh.contents->colors[num_colors].empty())
		{
			glGenBuffers(1, &a_mesh.contents->VBOs.emplace_back(0));
			glBindBuffer(GL_ARRAY_BUFFER, a_mesh.contents->VBOs[a_mesh.contents->VBOs.size() - 1]);
			glBufferData(GL_ARRAY_BUFFER, SIZEOF_ELEMENTS_IN_VECTOR(a_mesh.contents->colors[num_colors]), a_mesh.contents->colors[num_colors].data(), GL_STATIC_DRAW);
			glVertexAttribPointer((GLuint)((size_t)BufferAttributeLocation::Color0 + num_colors), 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glEnableVertexAttribArray((GLuint)((size_t)BufferAttributeLocation::Color0 + num_colors));
		}
	}
	size_t num_uvs = 0;
	for (; num_uvs < a_mesh.contents->uvs.size(); num_uvs++)
	{
		if (!a_mesh.contents->uvs[num_uvs].empty())
		{
			glGenBuffers(1, &a_mesh.contents->VBOs.emplace_back(0));
			glBindBuffer(GL_ARRAY_BUFFER, a_mesh.contents->VBOs[a_mesh.contents->VBOs.size() - 1]);
			glBufferData(GL_ARRAY_BUFFER, SIZEOF_ELEMENTS_IN_VECTOR(a_mesh.contents->uvs[num_uvs]), a_mesh.contents->uvs[num_uvs].data(), GL_STATIC_DRAW);
			glVertexAttribPointer((GLuint)((size_t)BufferAttributeLocation::UV0 + num_uvs), 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glEnableVertexAttribArray((GLuint)((size_t)BufferAttributeLocation::UV0 + num_uvs));
		}
	}
	a_mesh.vao_load_status = ELoadStatus::Loaded;
	return true;
}
