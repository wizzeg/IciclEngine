#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNrm;
layout (location = 4) in vec4 aCol;
layout (location = 10) in vec3 aTexCoord;

out vec4 vCol;
out vec3 vTexCoord;
out vec3 test_thing;
out vec3 vert_pos;
out vec3 vert_normal;
out vec3 vert_to_camera;

uniform mat4 transform;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform vec3 camera_position; // shouldn't this be camera positon?

void main()
{
	// phong shading
	


	//////////// old stuff
	//int vertexIndex = gl_VertexID;
	//projection * view * model *
	gl_Position = proj * view * model * vec4(aPos, 1.0);
	vTexCoord = aTexCoord.xyz;
	test_thing = camera_position;
	//vert_normal = (model * vec4(aNrm,1)).xyz;
	mat3 normalMatrix = transpose(inverse(mat3(model)));
	vert_normal = normalize(normalMatrix * aNrm); // rotate normal
	vert_pos = (model * vec4(aPos, 1)).xyz;
	vert_to_camera = normalize(camera_position - vert_pos);
	vec3 combined = aCol.rgb * 0.35 + (aNrm.rgb * 0.5 + 0.5) * 0.65;
	vCol = vec4(vert_normal, 1.0);
}
	