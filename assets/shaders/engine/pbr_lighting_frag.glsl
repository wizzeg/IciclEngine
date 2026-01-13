#version 460 core
out vec4 FragColor;

uniform sampler2D position_tex;
uniform sampler2D normal_tex;
uniform sampler2D albedo_spec_tex;
uniform sampler2D orms_tex;

in vec2 tex_coords;

uniform vec3 camera_position;
uniform vec3 light_positions[228];
uniform vec3 light_colors[228];
uniform vec3 light_attenuations[228];
uniform float light_intensities[228];
uniform int num_lights;
uniform vec3 light_attenuation = vec3(0.75, 0.1, 0.01);

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float num = a2 * NdotH * NdotH;
    float denom = (1.0 - NdotH * NdotH) + a2;
    denom = PI * denom * denom + 0.0000001;
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

void main() {
    vec4 albedo_spec = texture(albedo_spec_tex, tex_coords.xy);
    vec4 frag_pos = texture(position_tex, tex_coords.xy);
    vec4 frag_normal = texture(normal_tex, tex_coords.xy);
    vec4 orm = texture(orm_tex, tex_coords.xy);
    
    // PBR values from orm
    float ao = orm.x;
    float roughness = orm.y;
    float metallic = orm.z;
    float specular = albedo_spec.w;
    
    if (frag_pos.w < 0.5f) {
        FragColor = vec4(0.45f, 0.55f, 0.75f, 1.0f);
        return;
    }
    
    vec3 N = normalize(frag_normal.xyz);
    vec3 V = normalize(camera_position - frag_pos.xyz);
    vec3 albedo = albedo_spec.xyz;
    
    // PBR Fresnel base (dielectric F0 = 0.04)
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    
    // Ambient (IBL approximation)
    vec3 ambient = vec3(0.3) * albedo * ao;
    vec4 frag_color = vec4(ambient, 1.0);
    
    // Loop through your lights
    for (int i = 0; i < num_lights; i++) {
        vec3 L = normalize(light_positions[i] - frag_pos.xyz);
        float new_distance = length(light_positions[i] - frag_pos.xyz);
        float attenuation = (light_intensities[i] * 15.f) / (light_attenuations[i].x + light_attenuations[i].y * new_distance + light_attenuations[i].z * new_distance * new_distance);
        
        if (attenuation < 0.025) continue;
        
        vec3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);
        
        // Fresnel
        vec3 F = F0 + (vec3(1.0) - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);
        
        // Cook-Torrance BRDF terms
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 kD = vec3(1.0) - F;
        kD *= 1.0 - metallic;
        
        // PBR lighting equation
        vec3 radiance = light_colors[i] * attenuation;
        vec3 Lo = (kD * albedo / PI * NdotL + F * G * NDF / (4.0 * NdotL * max(dot(N, H), 0.0) + 0.0000001)) * radiance;
        
        frag_color.xyz += Lo * ao;
    }
    
    FragColor = vec4(frag_color.xyz, 1.0);
}
