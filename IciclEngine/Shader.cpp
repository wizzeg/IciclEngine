#pragma once
#include "Shader.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>
//#include <filesystem>
//
//Shader::Shader() {
//
//}

Shader::~Shader() {

}
Shader::Shader(const char* vertPath, const char* fragPath) {
	
	unsigned int VertexShader = LoadVertexShader(vertPath);
	unsigned int FragmentShader = LoadFragmentShader(fragPath);

	shaderProgram = glCreateProgram();

	glAttachShader(shaderProgram, VertexShader);
	glAttachShader(shaderProgram, FragmentShader);
	glLinkProgram(shaderProgram);

	glDeleteShader(VertexShader);
	glDeleteShader(FragmentShader);

	int result;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &result);
	if (!result)
	{
		char log[512];
		glGetProgramInfoLog(shaderProgram, 512, NULL, log);
		std::cout << " Failed to load shader program - \n" << log << std::endl;
	}
}

void Shader::Use() {
	glUseProgram(shaderProgram);
}

void Shader::Stop() {
	glUseProgram(0);
}


std::string Shader::LoadFromFile(const char* aPath)
{

	//std::filesystem::path currentPath = std::filesystem::current_path();
	//std::filesystem::path fullPath = currentPath / aPath;
	//std::ifstream file(fullPath);
	std::ifstream file(aPath);
	if (!file.is_open())
	{
		std::cerr << "Failed to open file: " << aPath << std::endl;
		return "";
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

unsigned int Shader::LoadShader(const char* aPath, GLenum shaderType)
{
	std::string shaderCodeString = LoadFromFile(aPath);
	const char* shaderCode = shaderCodeString.c_str();

	unsigned int shaderObject;
	shaderObject = glCreateShader(shaderType);
	glShaderSource(shaderObject, 1, &shaderCode, NULL);
	glCompileShader(shaderObject);

	int result;
	glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &result);

	if (!result)
	{
		char log[512];
		std::string type;
		switch (shaderType)
		{
		case GL_VERTEX_SHADER:
			type = "Vertex Shader";
			break;
		case GL_FRAGMENT_SHADER:
			type = "Fragment Shader";
			break;
		default:
			type = "Unknown";
			break;
		}
		glGetShaderInfoLog(shaderObject, 512, NULL, log);
		std::cout << "Failed to compile " << type << " - \n" << log << std::endl;
	}



	return shaderObject;

}

unsigned int Shader::LoadVertexShader(const char* aPath) {
	return LoadShader(aPath, GL_VERTEX_SHADER);
}

unsigned int Shader::LoadFragmentShader(const char* aPath) {
	return LoadShader(aPath, GL_FRAGMENT_SHADER);
}

void Shader::SetVec1f(float value, const char* location)
{
	glUniform1f(glGetUniformLocation(shaderProgram, location), value);
}
void Shader::SetVec2f(float value[2], const char* location)
{
	glUniform2f(glGetUniformLocation(shaderProgram, location), value[0], value[1]);
}
void Shader::SetVec3f(float value[3], const char* location)
{
	glUniform3f(glGetUniformLocation(shaderProgram, location), value[0], value[1], value[2]);
}
void Shader::SetVec4f(float value[4], const char* location)
{
	glUniform4f(glGetUniformLocation(shaderProgram, location), value[0], value[1], value[2], value[3]);
}

void Shader::SetVec1i(int value, const char* location)
{
	glUniform1i(glGetUniformLocation(shaderProgram, location), value);
}
void Shader::SetVec2i(int value[2], const char* location)
{
	glUniform2i(glGetUniformLocation(shaderProgram, location), value[0], value[1]);
}
void Shader::SetVec3i(int value[3], const char* location)
{
	glUniform3i(glGetUniformLocation(shaderProgram, location), value[0], value[1], value[2]);
}
void Shader::SetVec4i(int value[4], const char* location)
{
	glUniform4i(glGetUniformLocation(shaderProgram, location), value[0], value[1], value[2], value[3]);
}

void Shader::SetVec1ui(unsigned int value, const char* location)
{
	glUniform1ui(glGetUniformLocation(shaderProgram, location), value);
}
void Shader::SetVec2ui(unsigned int value[2], const char* location)
{
	glUniform2ui(glGetUniformLocation(shaderProgram, location), value[0], value[1]);
}
void Shader::SetVec3ui(unsigned int value[3], const char* location)
{
	glUniform3ui(glGetUniformLocation(shaderProgram, location), value[0], value[1], value[2]);
}
void Shader::SetVec4ui(unsigned int value[4], const char* location)
{
	glUniform4ui(glGetUniformLocation(shaderProgram, location), value[0], value[1], value[2], value[3]);
}