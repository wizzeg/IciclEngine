#version 460 core
out vec4 FragColor;

in vec2 tex_coords;
in vec4 color;

uniform sampler2D uCursor;

void main()
{
    vec4 texColor = texture(uCursor, tex_coords);
    if (texColor.w > 0.1f)
    {
        FragColor = texColor * color;
    }
    else
    {
        discard;
    }
}