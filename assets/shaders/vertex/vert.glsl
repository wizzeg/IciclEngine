#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aCol;

out vec3 vCol;

void main()
{
	int vertexIndex = gl_VertexID;
	gl_Position = vec4(aPos + (vertexIndex * 0.125), 1.0);
	vCol = aCol;
}
