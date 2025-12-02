#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNrm;
layout (location = 4) in vec4 aCol;
layout (location = 10) in vec3 aTexCoord;

out vec4 vCol;
out vec3 vTexCoord;

uniform mat4 transform;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main()
{
	//int vertexIndex = gl_VertexID;
	//projection * view * model *
	gl_Position = proj * view * model * vec4(aPos, 1.0);
	vCol = vec4(aCol.rgb, 1.0);
	vTexCoord = aTexCoord.xyz;
}
