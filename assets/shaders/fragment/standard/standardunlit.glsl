#version 460 core
out vec4 FragColor;

in vec3 vCol;
in vec2 vTexCoord;

uniform sampler2D uTexture0;

void main()
{
	//FragColor = vec4(vCol, 1.0);
	FragColor = texture(uTexture0, vTexCoord);
	//FragColor = vec4(vTexCoord, 0, 1.0);
}
