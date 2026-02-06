#version 460 core
layout (location = 0) out vec4 gPosition;    // world position
layout (location = 1) out vec4 gNormal;      // world normal
layout (location = 2) out vec4 gAlbedoSpec;  // RGB = albedo, A = specular
layout (location = 3) out vec4 gORMS;
layout (location = 4) out vec4 gSpec;
layout (location = 5) out vec4 gEmisive;

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
    gAlbedoSpec.rgb =  vec3(0.7f, 0.7f, 0.7f);//albedo;//forced_color * albedo.x;
    gAlbedoSpec.a = 1.0f; // a default specular
    gEmisive = vec4(0.7f, 0.7f, 0.7f, 1.0f);
}