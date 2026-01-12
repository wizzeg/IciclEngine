#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;


// wrong shader
//uniform sampler2DArray shadow_maps;
//
//struct shadow_caster
//{
//    
//    mat4 lighting_matrix;
//    vec3 light_direction;
//    uint high_res; // always keep 16bit alignment
//    vec4 light_color;
//}
//layout(std430, binding = 0) buffer ShadowCasterBuffer
//{
//    ShadowCaster shadow_casters[];
//};
//uniform uint num_shadow_casters;

uniform vec3 camera_position;
out vec2 tex_coords;

void main()
{
    tex_coords = aTexCoords.xy;
    gl_Position = vec4(aPos, 1.0);
}