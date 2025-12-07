#version 460 core
out vec4 FragColor;

in vec4 vCol;
in vec3 vTexCoord;

uniform sampler2D uTexture;

void main()
{
	FragColor = clamp(vCol + texture(uTexture, vTexCoord.xy), 0, 1);
	//FragColor = texture(uTexture, vTexCoord);
}
