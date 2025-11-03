#pragma once

#include "Mesh.h"
#include "Shader.h"
#include "Material.h"

class Renderable
{
public:
	Renderable();
	Renderable(Mesh* aMesh);
	Renderable(Mesh* aMesh, Shader* aShader);
	~Renderable();

	virtual void Render();
	virtual void DrawMesh();
	virtual void SetRenderable(Mesh* aMesh, Shader* aShader);
	virtual void SetMesh(Mesh* aMesh);
	virtual void SetShader(Shader* aShader);
	virtual void SetShaderProgram(unsigned int aShaderProgram);
	virtual unsigned int GetShaderProgram() { return shaderProgram; };
	
	Mesh* mesh = nullptr;
	Shader* shader = nullptr;
	Material* material = nullptr; //TODO: make renderable use a material instead
	unsigned int shaderProgram = 0;

	bool operator<(const Renderable& other) const
	{
		return shaderProgram < other.shaderProgram;
	}
	bool operator>(const Renderable& other) const
	{
		return shaderProgram > other.shaderProgram;
	}

	bool operator==(const Renderable& other) const
	{
		return shaderProgram == other.shaderProgram;
	}
};

