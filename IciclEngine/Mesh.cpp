#pragma once
#include "Mesh.h"
#include <numeric>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

Mesh::Mesh() : initialized(false), hasMeshData(false) { std::cout << "Mesh Empty!" << std::endl; }

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
	glBufferData(GL_ARRAY_BUFFER, (vertices.size() * sizeof(unsigned int)), (void*)vertices.data(), GL_STATIC_DRAW); // load vertices

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
	if (!initialized || !hasMeshData) return;
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	//glBindVertexArray(0);
}

Mesh::~Mesh() {
	if (initialized)
	{
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
	}
}