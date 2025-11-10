#pragma once
#include <glad/glad.h>
#include <string>
#include <functional>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
	const unsigned int GetShaderProgram() { return shaderProgram; };
	
	void SetVec1f(float value, const char* location);
	void SetVec2f(float value[2], const char* location);
	void SetVec3f(float value[3], const char* location);
	void SetVec4f(float value[4], const char* location);

	void SetVec1i(int value, const char* location);
	void SetVec2i(int value[2], const char* location);
	void SetVec3i(int value[3], const char* location);
	void SetVec4i(int value[4], const char* location);
	
	void SetVec1ui(unsigned int value, const char* location);
	void SetVec2ui(unsigned int value[2], const char* location);
	void SetVec3ui(unsigned int value[3], const char* location);
	void SetVec4ui(unsigned int value[4], const char* location);

	void SetMat4fv(glm::mat4 value, const char* location);

	//Uniforms setters

private:
	unsigned int shaderProgram = 0;
};