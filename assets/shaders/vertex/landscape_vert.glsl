#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNrm;
layout (location = 4) in vec4 aCol;
layout (location = 10) in vec3 aTexCoord;

out vec4 frag_pos;
out vec3 vert_normal;
out vec2 tex_coords;
out vec4 vert_color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

uniform vec3 forced_color;
uniform float landscape_height = 50.f;
uniform int vertex_count_x = 256;
uniform int vertex_count_z = 256;

uniform int instance_buffer = 0;
uniform sampler2D uTexture;

uniform mat4 light_space_matrix;
uniform int shadow_pass = 0;

struct ModelMatrix {
    mat4 model_matrix;
};

layout (std430, binding = 0) buffer ModelMatrixBuffer {
    int half_buffer_size; // don't need these...
    int num_model_matrices2;
	int[2] pad; 
    ModelMatrix model_matrices[];  // Unsized array
};

// thanks ai
vec3 calculate_heightmap_normal(vec2 uv, mat4 model_mat) {
    vec2 step = 1.0 / vec2(vertex_count_x, vertex_count_z); // one vertex apart in UV space

    float scale_x = length(model_mat[0].xyz);
    float scale_z = length(model_mat[2].xyz);
    float step_x = scale_x * step.x * 2.0;
    float step_z = scale_z * step.y * 2.0;

    float hL = texture(uTexture, uv + vec2(-step.x, 0)).r * landscape_height;
    float hR = texture(uTexture, uv + vec2( step.x, 0)).r * landscape_height;
    float hD = texture(uTexture, uv + vec2(0, -step.y)).r * landscape_height;
    float hU = texture(uTexture, uv + vec2(0,  step.y)).r * landscape_height;

    vec3 tangent_x = vec3(step_x, hR - hL, 0.0);
    vec3 tangent_z = vec3(0.0, hU - hD, step_z);
    return normalize(cross(tangent_z, tangent_x));
}

void main()
{
if (instance_buffer > 0)
    {
        vec3 instance_color = vec3(0);
        int instanceID = gl_InstanceID;
        if (instance_buffer == 2)
        {
            instanceID += half_buffer_size;
        }
        vec4 world_pos = model_matrices[instanceID].model_matrix * vec4(aPos, 1.0);

        if (shadow_pass > 0)
        {
            gl_Position = light_space_matrix * world_pos;
            return;
        }

        frag_pos = vec4(world_pos.xyz, 1);
        // vertex normal + rotation
        mat3 rotation_matrix = transpose(inverse(mat3(model_matrices[instanceID].model_matrix)));

        vert_normal = normalize(rotation_matrix * aNrm);

        tex_coords = aTexCoord.xy;
        vert_color = aCol;
        
        float height = texture(uTexture, tex_coords).r;
        world_pos.xyz +=  vec3(0, height * landscape_height, 0);
        // vertex world space
        frag_pos = vec4(world_pos.xyz, 1);
    
        // vertex normal + rotation
//        mat3 normal_matrix = transpose(inverse(mat3(model)));
//        vert_normal = normalize(normal_matrix * aNrm);
        mat3 normal_matrix = transpose(inverse(mat3(model_matrices[instanceID].model_matrix)));
        vec3 object_normal = calculate_heightmap_normal(tex_coords, model_matrices[instanceID].model_matrix);
        vert_normal = normalize(object_normal);

    
        // texture

        // fragment screen pos
        gl_Position = proj * view * world_pos;
        return;
    }



    // vertex world space
    vec4 world_pos = model * vec4(aPos, 1.0);

    if (shadow_pass > 0)
    {
        gl_Position = light_space_matrix * world_pos;
        return;
    }
    // vertex normal + rotation
    mat3 rotation_matrix = transpose(inverse(mat3(model)));
    vert_normal = normalize(rotation_matrix * aNrm);

    // texture
    tex_coords = aTexCoord.xy;
    vert_color = aCol;
    
    float height = texture(uTexture, tex_coords).r;
    world_pos.xyz +=  vec3(0, height * landscape_height,0);

    // vertex world space
    frag_pos = vec4(world_pos.xyz, 1);
    
    // vertex normal + rotation
//    mat3 normal_matrix = transpose(inverse(mat3(model)));
//    vert_normal = normalize(normal_matrix * aNrm);
    mat3 normal_matrix = transpose(inverse(mat3(model)));
    vec3 object_normal = calculate_heightmap_normal(tex_coords, model);
    vert_normal = normalize(object_normal);

    // fragment screen pos
    gl_Position = proj * view * world_pos;
}