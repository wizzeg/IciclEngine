#pragma once
#include "Triangle.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

Triangle::Triangle() {
	float vertices[] = {
	-0.5f, -0.5f, 0.0f, 1.0, 0.0, 0.0,
	0.5f, -0.5f, 0.0f, 0.0, 1.0, 0.0,
	0.0f, 0.5f, 0.0f, 0.0, 0.0, 1.0
	};

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);					// this is our raw data??? (I think is sets up a binder, and it basically stores all the things below)

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);		// the previous is my reference

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // load vertices

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (indices.size() * sizeof(unsigned int)), (void*)indices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0); // just enables the attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // wkmffhich attribute location, how many floats, that it is floats, stride, and offset
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Triangle::Render() {
	glBindVertexArray(VAO);
	//glDrawArrays(GL_TRIANGLES, 0, 3);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
}

Triangle::~Triangle() {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
}