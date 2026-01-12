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
uniform vec4 light_ambient = vec4(0.17, 0.15, 0.12, 1);
uniform vec4 light_diffuse = vec4(1, 0.9, 0.8, 1);
uniform vec4 light_specular = vec4(0.9, 0.85, 0.8, 1);
uniform vec3 light_position = vec3(0,0, 0);
uniform vec3 light_attenuation = vec3(1, 0.1, 0.001);

uniform vec4 material_diffuse = vec4(1, 1, 1, 1);
uniform vec4 material_ambient = vec4(1, 1, 1, 1);
uniform vec4 material_specular = vec4(1, 1, 1, 1);
uniform int material_shininess = 48;
const float PI = 3.14159265359;

void main()
{
	vec4 temp_color = texture(uTexture, vTexCoord.xy) * step(0.5f, has_texture) + normalize(vCol) * (1 - step(0.5f, has_texture));
	
//	if (has_texture > 0.001)
//	{
//		//FragColor = clamp(vCol * 0.4 + texture(uTexture, vTexCoord.xy) * 0.6, 0.0f, 1.0f);
//		temp_color = texture(uTexture, vTexCoord.xy);
//	}
//	else
//	{
	//	temp_color = vCol;
//	}
	vec4 frag_color = vec4(0.0, 0.0, 0.0, temp_color.w);
	
	frag_color += material_ambient * light_ambient;
	vec3 light_direction = normalize(light_position - vert_pos);
	vec3 normalized_normal = normalize(vert_normal);

	float diffuse_intensity = max(dot(normalized_normal, light_direction), 0.0);
	float multiplier = step(0.0, dot(normalized_normal, light_direction));
//	if (diffuse_intensity > 0.0)
//	{
		// diffuse
	vec3 diffuse_part = diffuse_intensity * light_diffuse.xyz * material_diffuse.xyz;
	float light_distance = length(light_position - vert_pos);
	float attenuation = 1.0 / (light_attenuation.x  + light_attenuation.y * light_distance  + light_attenuation.z * pow(light_distance, 2));
	frag_color += vec4(attenuation * diffuse_part, 0.0);
	//frag_color *= temp_color;

	// specular
	float mat_shininess = max(material_shininess, 1);
	vec3 vector_to_camera = normalize(camera_position - vert_pos);
	vec3 half_vector = normalize(light_direction + vector_to_camera);
	float initial_brightness = max(dot(half_vector, normalized_normal), 0.0);

	//float total_brightness = initial_brightness * pow(1.0039215686274509803921568627451, initial_brightness) * 1.25;
	float total_brightness = pow(initial_brightness, mat_shininess);
	//float area_normalization = (mat_shininess + 8.0) / (8.0 * 3.14159);
	float area_normalization = (mat_shininess + 3)*(mat_shininess + 6 ) / ((16 * PI )*(2-(mat_shininess/2) + mat_shininess));
	float light_absorption = clamp (mat_shininess / 16.0, 0.0, 1.0);
	//float max_brightness = pow(initial_brightness, mat_shininess + 128);
	//total_brightness += 0.01* total_brightness / max_brightness ;
	total_brightness *= light_absorption * area_normalization;
	frag_color += vec4(total_brightness * attenuation * diffuse_intensity * light_specular.xyz * material_specular.xyz, 0.0);
//	}
//	else
//	{
		//frag_color *= temp_color;
//	}
	frag_color *= temp_color;
	FragColor = clamp(frag_color, 0.0, 1.0);
	 // problem is probably that the color gets clamped or something...
	

//	vec3 normalized_normal = normalize(vert_normal);
//	vec4 frag_color = vec4(0.0, 0.0, 0.0, 1.0);
//	float light_distance = length(light_position - vert_pos);
//	float attenuation = 1.0 / (light_attenuation.x  + light_attenuation.y * light_distance  + light_attenuation.z * pow(light_distance, 2));
//	vec3 light_direction = normalize(light_position - vert_pos);
//	vec3 vector_to_camera = normalize(camera_position - vert_pos);
//	vec3 half_vector = normalize(light_direction + vector_to_camera);
//	float initial_brightness = max(dot(half_vector, normalized_normal), 0.0);
//
//	float total_brightness = pow(initial_brightness, float(material_shininess));
//	frag_color += (total_brightness * light_specular.xyz  * attenuation * material_specular.xyz, 0.0);
//	FragColor = total_brightness.xxxx;

	//FragColor = vec4(dot(normalize(vert_normal), normalize(camera_position - vert_pos)).xxx, 1);
	//FragColor = light_intensity * temp_color;
	//FragColor = vCol;
	
	//FragColor = texture(uTexture, vTexCoord);
}
