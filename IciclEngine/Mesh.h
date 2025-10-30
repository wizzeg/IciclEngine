#pragma once

#include <vector>

class Mesh
{
	unsigned int VBO = 0;
	unsigned int VAO = 0;
	unsigned int EBO = 0;
	bool initialized = false;
	bool hasMeshData = false;

	std::vector<unsigned int> indices;
	std::vector<float> vertices;
	std::vector<unsigned int> offsets;
public:
	void Render();
	void InitializeBuffers();

	Mesh();
	Mesh(std::vector<unsigned int>&& indicies, std::vector<float>&& verticies, std::vector<unsigned int>&& offsets);
	Mesh(const std::vector<unsigned int>& indicies, const std::vector<float>& verticies, const std::vector<unsigned int>& offsets);
	~Mesh();
};