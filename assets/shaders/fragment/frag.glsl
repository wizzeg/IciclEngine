#version 460 core
out vec4 FragColor;

in vec4 vCol;
in vec3 vTexCoord;

uniform sampler2D uTexture;
uniform int has_texture;

void main()
{
	if (has_texture > 0.001)
	{
		//FragColor = clamp(vCol * 0.4 + texture(uTexture, vTexCoord.xy) * 0.6, 0.0f, 1.0f);
		FragColor = texture(uTexture, vTexCoord.xy);
	}
	else
	{
		FragColor = vCol;
	}
	//FragColor = vCol;
	
	//FragColor = texture(uTexture, vTexCoord);
}
