#version 460 core
out vec4 FragColor;

in vec4 vCol;
in vec3 vTexCoord;
in vec3 vert_pos;
in vec3 vert_normal;
in vec3 vert_to_camera;

uniform sampler2D uTexture;
uniform int has_texture;
uniform vec3 camera_position;
uniform mat4 model;
uniform vec4 light_ambient = vec4(0.2, 0.2, 0.2, 1);
uniform vec4 light_diffuse = vec4(1, 0.9, 0.8, 1);
uniform vec4 light_specular = vec4(0.9, 0.8, 0.7, 1);
uniform vec3 light_position = vec3(1,1, 1);
uniform vec3 light_attenuation = vec3(1, 0.1, 0.001);

uniform vec4 material_diffuse = vec4(1, 1, 1, 1);
uniform vec4 material_ambient = vec4(1, 1, 1, 1);
uniform vec4 material_specular = vec4(1, 1, 1, 1);
uniform int material_shininess = 128;

void main()
{
	vec4 temp_color;
	if (has_texture > 0.001)
	{
		//FragColor = clamp(vCol * 0.4 + texture(uTexture, vTexCoord.xy) * 0.6, 0.0f, 1.0f);
		temp_color = texture(uTexture, vTexCoord.xy);
	}
	else
	{
		temp_color = vCol;
	}
	vec4 frag_color = vec4(0.0, 0.0, 0.0, temp_color.w);
	
	frag_color += material_ambient * light_ambient;
	vec3 light_direction = normalize(light_position - vert_pos);
	vec3 normalized_normal = normalize(vert_normal);

	float diffuse_intensity = max(dot(vert_normal, light_direction), 0.0);
	if (diffuse_intensity > 0.0)
	{
		// diffuse
		vec3 diffuse_part = diffuse_intensity * light_diffuse.xyz * material_diffuse.xyz;
		float light_distance = length(light_position - vert_pos);
		float attenuation = 1.0 / (light_attenuation.x  + light_attenuation.y * light_distance  + light_attenuation.z * pow(light_distance, 2));
		frag_color += vec4(diffuse_part * attenuation, 0.0);

		// specular
		vec3 vector_to_camera = normalize(camera_position - vert_pos);
		vec3 half_vector = normalize(light_direction + vector_to_camera);
		float initial_brightness = max(dot(half_vector, normalized_normal), 0.0);

		float total_brightness = pow(initial_brightness, float(material_shininess));
//		for (int i = 0; i < material_shininess; ++i)
//		{
//			total_brightness *= initial_brightness;
//		}
		frag_color += (total_brightness * light_specular.xyz * attenuation * material_specular.xyz, 0.0);
	}
	frag_color *= temp_color;
	FragColor = frag_color;

	//FragColor = vec4(dot(normalize(vert_normal), normalize(camera_position - vert_pos)).xxx, 1);
	//FragColor = light_intensity * temp_color;
	//FragColor = vCol;
	
	//FragColor = texture(uTexture, vTexCoord);
}
