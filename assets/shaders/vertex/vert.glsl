#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 4) in vec3 aCol;
layout (location = 10) in vec2 aTexCoord;

out vec3 vCol;
out vec2 vTexCoord;

uniform mat4 transform;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	//int vertexIndex = gl_VertexID;
	//projection * view * model *
	gl_Position = projection * view * model * vec4(aPos, 1.0);
	vCol = aCol;
	vTexCoord = aTexCoord;
}
