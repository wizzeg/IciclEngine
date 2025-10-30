#pragma once
#include <glad/glad.h>
#include <string>


class Shader {
public:
	//Shader();
	Shader(const char* vertPath, const char* fragPath);
	~Shader();

	void Use();
	void Recompile() {}; // later
	void Stop();

	std::string LoadFromFile(const char* aPath);

	unsigned int LoadVertexShader(const char* aPath);
	unsigned int LoadFragmentShader(const char* aPath);
	unsigned int LoadShader(const char* aPath, GLenum shaderType);
	unsigned int GetShaderProgram() { return shaderProgram; }

	//Uniforms setters

private:
	unsigned int shaderProgram = 0;
};