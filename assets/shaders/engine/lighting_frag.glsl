#version 460 core
out vec4 FragColor;

uniform sampler2D position_tex;
uniform sampler2D normal_tex;
uniform sampler2D albedo_spec_tex;
uniform sampler2D orms_tex;

//struct point_light
//{
//    vec4 world_pos;
//    vec4 light_color;
//}
//
//layout(std430, binding = 0) buffer point_lights_buffer
//{
//    point_light point_lights[];
//};
//uniform uint num_point_lights;
//


in vec2 tex_coords; // we'll need this for lookup on the gbuffer

uniform vec3 camera_position;

uniform vec4 light_ambient = vec4(0.17, 0.15, 0.12, 1);
uniform vec4 light_diffuse = vec4(1, 0.9, 0.8, 1);
uniform vec4 light_specular = vec4(0.9, 0.85, 0.8, 1);
uniform vec3 light_position = vec3(0,0, 0);
uniform vec3 light_attenuation = vec3(0.75, 0.1, 0.01);

uniform vec4 material_diffuse = vec4(1, 1, 1, 1);
uniform vec4 material_ambient = vec4(1, 1, 1, 1);
uniform vec4 material_specular = vec4(1, 1, 1, 1);
uniform int material_shininess = 1000;

uniform vec3 light_positions[228];
uniform vec3 light_colors[228];
uniform vec3 light_attenuations[228];
uniform float light_intensities[228];
uniform int num_lights;

const float PI = 3.14159265359;



void main()
{
	vec4 albedo_spec = texture(albedo_spec_tex, tex_coords.xy);
	vec4 frag_pos = texture(position_tex, tex_coords.xy);
	vec4 frag_normal = texture(normal_tex, tex_coords.xy);
	vec4 orms = texture(orms_tex, tex_coords.xy);

	float roughness = orms.y;
	float metallic = orms.z;
	float ao = orms.x;
	float shininess_multiplier = orms.w;
	float shininess =  metallic * shininess_multiplier * material_shininess;

	float attenuation = 0;
	vec4 frag_color = vec4(vec3(0), albedo_spec.w);
	if (frag_pos.w < 0.5f)
	{
		//glClearColor(0.45f, 0.55f, 0.75f, 0.0f);
		FragColor = vec4(0.45f, 0.55f, 0.75f, 1.0f);
		return;
	}
	frag_color.xyz += albedo_spec.xyz * light_ambient.xyz;
	for (int i = 0; i < num_lights; i++)
	{ 
		vec3 light_direction = normalize(light_positions[i] - frag_pos.xyz);
		float light_distance = length(light_positions[i] - frag_pos.xyz);
		vec3 normalized_normal = normalize(frag_normal.xyz);

		attenuation = ( light_intensities[i]) / (  (light_attenuations[i].x  + light_attenuations[i].y * light_distance  + light_attenuations[i].z * pow(light_distance, 2)));
		float diffuse_intensity = dot(normalized_normal, light_direction);
		if (diffuse_intensity > 0 && attenuation > 0.025)
		{
			// diffuse
			vec3 diffuse_part = diffuse_intensity * light_colors[i].xyz * albedo_spec.xyz * ao;
			
			frag_color += vec4(attenuation * diffuse_part, 0.0);

			// specular
			float mat_shininess = (1 - roughness) * max(shininess, 1);
			vec3 vector_to_camera = normalize(camera_position - frag_pos.xyz);
			vec3 half_vector = normalize(light_direction + vector_to_camera);
			float initial_brightness = max(dot(half_vector, normalized_normal), 0.0);

			// calculate total brightness, and normalize it a bit through absorption (if it's not very shiny), and increase power if it's very shiny
			float total_brightness = pow(initial_brightness, mat_shininess);
			float area_normalization = (mat_shininess + 3)*(mat_shininess + 6 ) / ((16 * PI )*(2-(mat_shininess/2) + mat_shininess));
			float light_absorption = metallic * clamp(mat_shininess / 32.0, 0.0, 1.0);
			total_brightness *= light_absorption * area_normalization * roughness;
			frag_color += vec4(total_brightness * attenuation * diffuse_intensity * light_colors[i].xyz * albedo_spec.xyz, 0.0) * frag_pos.w;
		}
		
	}
	frag_color *= albedo_spec;
	//frag_color -= vec4(vec3(0.25f) * (metallic * metallic), 0);
	FragColor = clamp(frag_color, 0.0, 1.0);
	//FragColor = vec4(orm.yyy, 1);
	//FragColor = vec4(attenuation.xxx, 1.0);
	//FragColor = vec4(light_colors[1].xyz, 1.0);
	//}
}

//void main()
//{
//	
//	vec4 albedo_spec = texture(albedo_spec_tex, tex_coords.xy);
//	vec4 frag_pos = texture(position_tex, tex_coords.xy);
//	vec4 frag_normal = texture(normal_tex, tex_coords.xy);
//	vec4 orms = texture(orm_tex, tex_coords.xy);
//
//	float shininess =  orms.z * material_shininess;
//
//	float attenuation = 0;
//	vec4 frag_color = vec4(vec3(0), albedo_spec.w);
//	if (frag_pos.w < 0.5f)
//	{
//		//glClearColor(0.45f, 0.55f, 0.75f, 0.0f);
//		FragColor = vec4(0.45f, 0.55f, 0.75f, 1.0f);
//		return;
//	}
//	frag_color.xyz += albedo_spec.xyz * light_ambient.xyz * orms.x;
//	for (int i = 0; i < num_lights; i++)
//	{ 
//		vec3 light_direction = normalize(light_positions[i] - frag_pos.xyz);
//		float light_distance = length(light_positions[i] - frag_pos.xyz);
//		vec3 normalized_normal = normalize(frag_normal.xyz);
//
//		attenuation = ( light_intensities[i]) / (  (light_attenuation.x  + light_attenuation.y * light_distance  + light_attenuation.z * pow(light_distance, 2)));
//		float diffuse_intensity = dot(normalized_normal, light_direction);
//		if (diffuse_intensity > 0 && attenuation > 0.025)
//		{
//			// diffuse
//			vec3 diffuse_part = diffuse_intensity * light_colors[i].xyz * albedo_spec.xyz;
//			
//			frag_color += vec4(attenuation * diffuse_part, 0.0);
//
//			// specular
//			float mat_shininess = max(shininess, 1);
//			vec3 vector_to_camera = normalize(camera_position - frag_pos.xyz);
//			vec3 half_vector = normalize(light_direction + vector_to_camera);
//			float initial_brightness = max(dot(half_vector, normalized_normal), 0.0);
//
//			// calculate total brightness, and normalize it a bit through absorption (if it's not very shiny), and increase power if it's very shiny
//			float total_brightness = pow(initial_brightness, mat_shininess);
//			float area_normalization = (mat_shininess + 3)*(mat_shininess + 6 ) / ((16 * PI )*(2-(mat_shininess/2) + mat_shininess));
//			float light_absorption = orms.y * clamp(mat_shininess / 32.0, 0.0, 1.0);
//			total_brightness *= light_absorption * area_normalization;
//			frag_color += vec4(total_brightness * attenuation * diffuse_intensity * light_colors[i].xyz * albedo_spec.xyz, 0.0) * frag_pos.w;
//		}
//		
//	}
//	frag_color *= albedo_spec;
//	FragColor = clamp(frag_color, 0.0, 1.0);
//	//FragColor = vec4(orm.zzz, 1);
//	//FragColor = vec4(attenuation.xxx, 1.0);
//	//FragColor = vec4(light_colors[1].xyz, 1.0);
//	//}
//}