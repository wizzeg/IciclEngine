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

void main()
{
    // vertex world space
    vec4 world_pos = model * vec4(aPos, 1.0);
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