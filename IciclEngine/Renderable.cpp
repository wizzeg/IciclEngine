#include "Renderable.h"
#include <print>

Renderable::Renderable() {}
Renderable::Renderable(Mesh* aMesh) : mesh(aMesh) {}
Renderable::Renderable(Mesh* aMesh, Shader* aShader) : mesh(aMesh), shader(aShader) { shaderProgram = shader->GetShaderProgram(); }

Renderable::~Renderable(){}

void Renderable::SetRenderable(Mesh* aMesh, Shader* aShader)
{
	mesh = aMesh;
	shader = aShader;
	shaderProgram = shader->GetShaderProgram();
}
void Renderable::SetMesh(Mesh* aMesh)
{
	mesh = aMesh;
}

void Renderable::SetShaderProgram(unsigned int aShaderProgram)
{
	shaderProgram = aShaderProgram;
}

void Renderable::SetShader(Shader* aShader)
{
	shader = aShader;
	shaderProgram = shader->GetShaderProgram();
}

void Renderable::Render()
{
	if (mesh == nullptr || shader == nullptr)
	{
		std::println("mesh or shader missing");
		return;
	}
	shader->Use();
	mesh->Render();
	shader->Stop();
}

void Renderable::DrawMesh()
{
	if (mesh == nullptr)
	{
		std::println("mesh missing");
		return;
	}
	mesh->Render();
}