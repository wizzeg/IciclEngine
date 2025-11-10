#pragma once
#include "Material.h"
#include "Shader.h"
#include "Texture.h"
#include <print>


Material::Material(Shader* shader, std::vector<class Texture*> textures) : shader(shader), textures(textures) { shaderProgram = shader->GetShaderProgram(); }

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
}

void Material::StopShader()
{
	shader->Stop();
}

void Material::Apply()
{
	if (updateUniforms) updateUniforms();
}