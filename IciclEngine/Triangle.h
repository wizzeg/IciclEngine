#pragma once

#include "Renderable.h"
#include <vector>

class Triangle : public Renderable
{
	unsigned int VBO;
	unsigned int VAO;
	unsigned int EBO;

	std::vector<unsigned int> indices = { 0, 1, 2 };
	//unsigned int indices[3]
	//{
	//	0, 1, 2
	//};
	//unsigned int* indicies;
	//unsigned int* test;
public:
	void Render() override;
	Triangle();
	~Triangle();
};