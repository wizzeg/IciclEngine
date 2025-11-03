#pragma once
#include <unordered_map>
#include <functional>

class Material
{
public:
	int shaderProgram = 0;
	std::function<void()> updateUniforms;
	std::unordered_map<const char*, int> uniformLocationCache;
	class Shader* shader;
	std::vector<class Texture*> textures;

	Material(class Shader* shader, std::vector<class Texture*> textures);

	//int AddUniform(const char* name);
	int GetOrAddUniform(const char* name);
	bool RemoveUniform(const char* name);
	void SetUpdateUniforms(std::function<void()> function);

	bool changeTexture(int index, class Texture* texture);
	
	void UseShader();
	void StopShader();
	void Apply();
};