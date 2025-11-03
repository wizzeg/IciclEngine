#version 460 core
out vec4 FragColor;

in vec3 vCol;
in vec2 vTexCoord;

uniform sampler2D uTexture;

void main()
{
	FragColor = texture(uTexture, vTexCoord);
}
