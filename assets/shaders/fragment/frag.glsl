#version 460 core
out vec4 FragColor;

in vec3 vCol;

void main()
{
	FragColor = vec4(vCol, 1);
}
