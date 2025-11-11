#pragma once
#include <unordered_map>
#include <functional>
#include "Texture.h"

class Material
{
public:
	int shaderProgram = 0;
	std::function<void()> updateUniforms;
	std::unordered_map<const char*, int> uniformLocationCache;
	class Shader* shader = nullptr;
	std::vector<class Texture*> textures;

	Material(class Shader* shader);


	//int AddUniform(const char* name);
	int GetOrAddUniform(const char* name);
	bool RemoveUniform(const char* name);

	unsigned int GetShaderProgram();

	void SetUpdateUniforms(std::function<void()> function);
	bool changeTexture(int index, class Texture* texture);
	
	void UseShader();
	void StopShader();
	void Apply();
	void UpdatePosition();
	void SetUniformTexture();
	bool AddTexture(Texture* texture);
	void InitializeTexutre(int index);
	void ActivateTexutres();
};