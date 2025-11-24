#include <engine/renderer/vao_loader.h>
#include <engine/utilities/macros.h>
#include <glfw/glfw3.h>
#include <glad/glad.h>

bool VAOLoader::load_vao(MeshData& a_mesh)
{
	if (a_mesh.bad_load)
	{
		PRINTLN("mesh has had a bad load");
		return false;
	}

	if (a_mesh.VAO != 0)
	{
		PRINTLN("VAO taken, clearing");
		for (size_t i = 0; i < a_mesh.VBOs.size(); i++)
		{
			glDeleteBuffers(1, &a_mesh.VBOs[i]);
		}
		a_mesh.VBOs.clear();
		glDeleteBuffers(1, &a_mesh.EBO);
		a_mesh.EBO = 0;
		glDeleteBuffers(1, &a_mesh.VAO);
		a_mesh.VAO = 0;
		a_mesh.VAO_loaded = false;
	}
	PRINTLN("loading mesh: {}", a_mesh.path);
	glGenVertexArrays(1, &a_mesh.VAO);
	glBindVertexArray(a_mesh.VAO);

	glGenBuffers(1, &a_mesh.EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, a_mesh.EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, SIZEOF_ELEMENTS_IN_VECTOR(a_mesh.indices), a_mesh.indices.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &a_mesh.VBOs.emplace_back(0));
	glBindBuffer(GL_ARRAY_BUFFER, a_mesh.VBOs[a_mesh.VBOs.size() - 1]);
	glBufferData(GL_ARRAY_BUFFER, SIZEOF_ELEMENTS_IN_VECTOR(a_mesh.positions), a_mesh.positions.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(BufferAttributeLocation::Position, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(BufferAttributeLocation::Position);
	
	if (!a_mesh.normals.empty())
	{
		glGenBuffers(1, &a_mesh.VBOs.emplace_back(0));
		glBindBuffer(GL_ARRAY_BUFFER, a_mesh.VBOs[a_mesh.VBOs.size() - 1]);
		glBufferData(GL_ARRAY_BUFFER, SIZEOF_ELEMENTS_IN_VECTOR(a_mesh.normals), a_mesh.normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(BufferAttributeLocation::Normal, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(BufferAttributeLocation::Normal);

	}
	if (!a_mesh.tangets.empty())
	{
		glGenBuffers(1, &a_mesh.VBOs.emplace_back(0));
		glBindBuffer(GL_ARRAY_BUFFER, a_mesh.VBOs[a_mesh.VBOs.size() - 1]);
		glBufferData(GL_ARRAY_BUFFER, SIZEOF_ELEMENTS_IN_VECTOR(a_mesh.tangets), a_mesh.tangets.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(BufferAttributeLocation::Tangent, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(BufferAttributeLocation::Tangent);
	}
	if (!a_mesh.bitangents.empty())
	{
		glGenBuffers(1, &a_mesh.VBOs.emplace_back(0));
		glBindBuffer(GL_ARRAY_BUFFER, a_mesh.VBOs[a_mesh.VBOs.size() - 1]);
		glBufferData(GL_ARRAY_BUFFER, SIZEOF_ELEMENTS_IN_VECTOR(a_mesh.bitangents), a_mesh.bitangents.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(BufferAttributeLocation::Bitangent, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(BufferAttributeLocation::Bitangent);
	}

	size_t num_colors = 0;
	for (; num_colors < a_mesh.colors.size(); num_colors++)
	{
		if (!a_mesh.colors[num_colors].empty())
		{
			glGenBuffers(1, &a_mesh.VBOs.emplace_back(0));
			glBindBuffer(GL_ARRAY_BUFFER, a_mesh.VBOs[a_mesh.VBOs.size() - 1]);
			glBufferData(GL_ARRAY_BUFFER, SIZEOF_ELEMENTS_IN_VECTOR(a_mesh.colors[num_colors]), a_mesh.colors[num_colors].data(), GL_STATIC_DRAW);
			glVertexAttribPointer(BufferAttributeLocation::Color0 + num_colors, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glEnableVertexAttribArray(BufferAttributeLocation::Color0 + num_colors);
		}
	}
	size_t num_uvs = 0;
	for (; num_uvs < a_mesh.uvs.size(); num_uvs++)
	{
		if (!a_mesh.uvs[num_uvs].empty())
		{
			glGenBuffers(1, &a_mesh.VBOs.emplace_back(0));
			glBindBuffer(GL_ARRAY_BUFFER, a_mesh.VBOs[a_mesh.VBOs.size() - 1]);
			glBufferData(GL_ARRAY_BUFFER, SIZEOF_ELEMENTS_IN_VECTOR(a_mesh.uvs[num_uvs]), a_mesh.uvs[num_uvs].data(), GL_STATIC_DRAW);
			glVertexAttribPointer(BufferAttributeLocation::UV0 + num_uvs, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glEnableVertexAttribArray(BufferAttributeLocation::UV0 + num_uvs);
		}
	}
	a_mesh.VAO_loaded = true;
	return true;
}
