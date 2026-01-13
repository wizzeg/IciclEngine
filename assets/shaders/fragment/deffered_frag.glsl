#version 460 core
layout (location = 0) out vec4 gPosition;    // world position
layout (location = 1) out vec4 gNormal;      // world normal
layout (location = 2) out vec4 gAlbedoSpec;  // RGB = albedo, A = specular
layout (location = 3) out vec4 gORMS;

in vec4 frag_pos;
in vec3 vert_normal;
in vec2 tex_coords;
in vec4 vert_color;
in mat3 TBN_matrix;

uniform sampler2D uTexture; // change this name later
uniform sampler2D uNormalMap;
uniform sampler2D uAO;
uniform sampler2D uRoughness;
uniform sampler2D uMetallic;

uniform float max_shininess = 1.0;
uniform float heightScale = 0.0001;
uniform int has_texture;


void main()
{
    gPosition.xyzw = frag_pos;
    gPosition.w = 1.0;
    //gNormal = vec4(normalize(vert_normal), 0.5);
    vec3 normal_map = texture(uNormalMap, tex_coords).rgb;
    normal_map = normalize(normal_map * 2.0 - 1.0);
    normal_map = normalize(mix(vec3(0, 0, 1), normal_map, 1.0f));

    float metallic = texture(uMetallic, tex_coords).r;
    float AO = texture(uAO, tex_coords).r;
    float roughness = texture(uRoughness, tex_coords).r;
    gORMS = vec4(AO, roughness, metallic, max_shininess);

    vec3 world_normal = normalize(TBN_matrix * normal_map);
    gNormal = vec4(world_normal, 1.0f);
    //gNormal = texture(uNormalMap, tex_coords);
    
    vec3 albedo;
//    if (has_texture > 0) {
    albedo = texture(uTexture, tex_coords).rgb;
//    } else {
//        albedo = vert_color.rgb;
//    }
    gAlbedoSpec.rgb = albedo;
    gAlbedoSpec.a = 1.0f; // mark pixel is active
}