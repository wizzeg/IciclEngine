#include "Renderable.h"
#include <print>

Renderable::Renderable() {}
Renderable::Renderable(Mesh* aMesh) : mesh(aMesh) {}
Renderable::Renderable(Mesh* aMesh, Shader* aShader) : mesh(aMesh), shader(aShader) { shaderProgram = shader->GetShaderProgram(); }
Renderable::Renderable(Mesh* aMesh, Material* aMaterial): mesh(aMesh), material(aMaterial) { }
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
	if (mesh == nullptr)
	{
		return;
	}
	
	if (material != nullptr)
	{
		material->UseShader();
		ChangeModelPosition();
		mesh->Render();
		material->StopShader();
	}
	else if (shader != nullptr)
	{
		shader->Use();
		mesh->Render();
		shader->Stop();
	}
	else
	{
		std::println("no shader or material assigned");
	}
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

void Renderable::ChangeModelPosition()
{
	glm::mat4 model = glm::mat4(1.0f);

	model = glm::translate(model, *position);

	model = glm::rotate(model, glm::radians((*rotation).x), glm::vec3(1, 0, 0));
	model = glm::rotate(model, glm::radians((*rotation).y), glm::vec3(0, 1, 0));
	model = glm::rotate(model, glm::radians((*rotation).z), glm::vec3(0, 0, 1));
	//std::println("rotaion in model positoin = {}", (*rotation).y);
	model = glm::scale(model, *scale);

	material->shader->SetMat4fv(model, "model");
}

unsigned int Renderable::GetShaderProgram()
{
	unsigned int result = 0;
	if (shader != nullptr)
	{
		result = shader->GetShaderProgram();
	}
	return result;
}