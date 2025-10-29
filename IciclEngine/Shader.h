#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glad/glad.h>


class Shader {
public:
	Shader();
	Shader(const char* vertPath, const char* fragPath);

	void Use();
	void Recompile() {}; // later
	void Stop();

	std::string LoadFromFile(const char* aPath);

	unsigned int LoadVertexShader(const char* aPath);
	unsigned int LoadFragmentShader(const char* aPath);
	unsigned int LoadShader(const char* aPath, GLenum shaderType);

	//Uniforms setters

private:
	unsigned int shaderProgram;
};