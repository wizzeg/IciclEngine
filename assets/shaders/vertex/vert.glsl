#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aCol;
layout (location = 2) in vec2 aTexCoord;

out vec3 vCol;
out vec2 vTexCoord;

void main()
{
	int vertexIndex = gl_VertexID;
	gl_Position = vec4(aPos + (vertexIndex * 0.125), 1.0);
	vCol = aCol;
	vTexCoord = aTexCoord;
}
