#version 460 core
layout (location = 0) out vec4 gPosition;    // World position
layout (location = 1) out vec4 gNormal;      // World normal
layout (location = 2) out vec4 gAlbedoSpec;  // RGB = albedo, A = specular
layout (location = 3) out vec4 gORMS;

in vec4 frag_pos;
in vec3 vert_normal;
in vec2 tex_coords;
in vec4 vert_color;

uniform vec3 forced_color;
uniform sampler2D uTexture; // change this name later
uniform int has_texture;

void main()
{
    gPosition.xyzw = frag_pos;
    //gPosition.w = 1.0;
    gNormal = vec4(normalize(vert_normal), 0.5);
    
    vec3 albedo;
//    if (has_texture > 0) {
    albedo = texture(uTexture, tex_coords).rgb;
//    } else {
//        albedo = vert_color.rgb;
//    }
    gAlbedoSpec.rgb = forced_color * albedo.x;
    gAlbedoSpec.a = 1.0f; // a default specular
}