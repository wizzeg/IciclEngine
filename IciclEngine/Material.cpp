#pragma once
#include "Material.h"
#include "Shader.h"
#include "Texture.h"
#include <print>


Material::Material(Shader* shader) : shader(shader) { shaderProgram = shader->GetShaderProgram(); }

//int Material::AddUniform(const char* name)
//{
//	if (uniformLocationCache.find(name) != uniformLocationCache.end())
//	{
//		return uniformLocationCache[name];
//	}
//	int location = glGetUniformLocation(shader->GetShaderProgram(), name);
//	uniformLocationCache[name] = location;
//	return location;
//}
int Material::GetOrAddUniform(const char* name)
{
	if (uniformLocationCache.find(name) != uniformLocationCache.end())
	{
		return uniformLocationCache[name];
	}
	int location = glGetUniformLocation(shader->GetShaderProgram(), name);
	uniformLocationCache[name] = location;
	return location;
}

bool Material::RemoveUniform(const char* name)
{
	bool result = (uniformLocationCache.find(name) != uniformLocationCache.end());
	if (result) uniformLocationCache.erase(name);
	return result;
}

void Material::SetUpdateUniforms(std::function<void()> function)
{
	updateUniforms = function;
}

bool Material::changeTexture(int index, Texture* texture)
{
	if (textures.size() > index)
		textures.at(index) = texture;
	else return false;
	return true;
}

void Material::UseShader()
{
	shader->Use();
	ActivateTexutres();
}

void Material::StopShader()
{
	shader->Stop();
}

void Material::Apply()
{
	//if (updateUniforms) updateUniforms();
	
}

void Material::UpdatePosition()
{

}
void Material::SetUniformTexture()
{
	if (textures.size() > 0)
	{
		std::string baseName = "uTexture";
		std::string number;
		std::string name;
		for (size_t i = 0; i < textures.size(); i++)
		{
			number = std::to_string(i);	
			name = baseName + number;
			textures[i]->Activate();
		}
	}
}

bool Material::AddTexture(Texture* texture)
{
	if (shaderProgram == 0)
	{
		return false;
	}
	for (size_t i = 0; i < textures.size(); i++)
	{
		if (textures[i]->Path == texture->Path)
		{
			return false;
		}
	}
	textures.push_back(texture);
	InitializeTexutre(((int)textures.size() -1));
	return true;
}

void Material::InitializeTexutre(int index)
{
	glUseProgram(shaderProgram);
	std::string name = "uTexture" + std::to_string(index);
	std::println("set the texture ID: {} {}", textures[index]->TextureID, name);
	shader->SetVec1ui(textures[index]->TextureID, name.c_str());

	glUseProgram(0);
}

void Material::ActivateTexutres()
{
	for (size_t i = 0; i < textures.size(); i++)
	{
		textures[i]->Activate();
	}
}

unsigned int Material::GetShaderProgram() { if (shader != nullptr) { return shader->GetShaderProgram(); } else return 0; }