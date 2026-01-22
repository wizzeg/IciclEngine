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

uniform int instance_buffer = 0;

uniform mat4 light_space_matrix;
uniform int shadow_pass = 0;

struct ModelMatrix {
    mat4 model_matrix;
};

layout (std430, binding = 0) buffer ModelMatrixBuffer {
    int num_model_matrices1; // don't need these...
    int num_model_matrices2;
	int[2] pad;
    ModelMatrix model_matrices[];  // Unsized array
};

void main()
{
if (instance_buffer > 0)
    {
        vec3 instance_color = vec3(0);
        int instanceID = gl_InstanceID;
        if (instance_buffer == 2)
        {
            instanceID += 1024;
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

        // vertex world space
        frag_pos = vec4(world_pos.xyz, 1);
    
        // vertex normal + rotation
        mat3 normal_matrix = transpose(inverse(mat3(model)));
        vert_normal = normalize(normal_matrix * aNrm);
    
        // texture
        tex_coords = aTexCoord.xy;
        vert_color = aCol;
    
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

    // vertex world space
    frag_pos = vec4(world_pos.xyz, 1);
    
    // vertex normal + rotation
    mat3 normal_matrix = transpose(inverse(mat3(model)));
    vert_normal = normalize(normal_matrix * aNrm);
    
    // texture
    tex_coords = aTexCoord.xy;
    vert_color = aCol;
    
    // fragment screen pos
    gl_Position = proj * view * world_pos;
}