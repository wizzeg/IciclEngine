#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNrm;
layout (location = 2) in vec4 aTngt;
layout (location = 4) in vec4 aCol;
layout (location = 10) in vec3 aTexCoord;

out vec4 frag_pos;
out vec3 vert_normal;
out vec2 tex_coords;
out vec4 vert_color;
out mat3 TBN_matrix;


uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

uniform int instance_buffer = 0;

uniform mat4 light_space_matrix;
uniform int shadow_pass = 0;

struct ModelMatrix {
    mat4 model_matrix;
};

layout (std430, binding = 0) buffer ModelMatrixBuffer {
    int half_buffer_size; // don't need these...
    int num_model_matrices2;
	int[2] pad;
    ModelMatrix model_matrices[]; 
};

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
    
        vec3 normal = normalize(rotation_matrix * aNrm);
        vec3 tangent = normalize(rotation_matrix * aTngt.xyz);
        vec3 bitangent = normalize(cross(normal, tangent) * aTngt.w);
        vert_normal = normal;
        TBN_matrix = mat3(tangent, bitangent, normal);

        // texture
        tex_coords = aTexCoord.xy;
        vert_color = aCol;
        if (instanceID >= 1024)
        {
            vert_color = vec4(1, 0, 0, 1);
        }
        else
        {
            vert_color = vec4(0, 1, 0, 1);
        }
        
    
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
    frag_pos = vec4(world_pos.xyz, 1);
    
    // vertex normal + rotation
    mat3 rotation_matrix = transpose(inverse(mat3(model)));
    
    vec3 normal = normalize(rotation_matrix * aNrm);
    vec3 tangent = normalize(rotation_matrix * aTngt.xyz);
    vec3 bitangent = normalize(cross(normal, tangent) * aTngt.w);
    vert_normal = normal;
    TBN_matrix = mat3(tangent, bitangent, normal);

    // texture
    tex_coords = aTexCoord.xy;
    vert_color = aCol;
    
    // fragment screen pos
    gl_Position = proj * view * world_pos;
}