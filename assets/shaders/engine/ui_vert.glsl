#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 tex_coords;
out vec4 color;

uniform int instance_buffer = 0;
//uniform int half_buffer_size = 2048;

struct UISSBO {
    vec4 color;
	vec2 uv_offset;
    vec2 uv_size;
	vec2 position;
	vec2 size;
};

layout (std430, binding = 2) buffer UISSBOs {
    int half_buffer_size;
    int pad1;
    int pad2;
    int pad3;
    UISSBO uis[]; 
};

void main()
{
    if (instance_buffer > 0)
    {
        int instanceID = gl_InstanceID;
        if (instance_buffer == 2)
        {
            instanceID += half_buffer_size;
        }
        
        UISSBO ui = uis[instanceID];
        // Get current vertex position
        vec2 vertex_pos = aPos.xy * ui.size + ui.position; //quad_vertices[gl_VertexID];
        
        // Calculate UV coordinates
        vec2 vertex_uv = aTexCoords * ui.uv_size + ui.uv_offset;
        
        // Set outputs
        tex_coords = vertex_uv;
        color = ui.color;
        gl_Position = vec4(vertex_pos, 0.0, 1.0);
        return;
    }
    gl_Position = vec4(0);
}