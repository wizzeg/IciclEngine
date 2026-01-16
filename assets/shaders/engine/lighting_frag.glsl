#version 460 core
out vec4 FragColor;

uniform sampler2D position_tex;
uniform sampler2D normal_tex;
uniform sampler2D albedo_spec_tex;
uniform sampler2D orms_tex;
uniform sampler2D emissive_tex;
uniform sampler2D spec_tex;

uniform sampler2DArray shadow_maps;
uniform mat4 light_space_matrix[10];
uniform vec3 light_positions[10];
uniform vec4 light_colors[10];
uniform vec4 light_intencities[10];
uniform vec3 light_attenuations[10];
uniform int light_tex_id[10];
uniform int num_shadow_maps;


struct PointLight {
	vec4 color;
    vec4 position;
    vec4 attenuation;
};

layout (std430, binding = 1) buffer PointLightBuffer {
    int num_pointlights;
	int[3] pad;
    PointLight point_lights[];  // Unsized array
};

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
uniform int material_shininess = 500;



const float PI = 3.14159265359;

//
void main()
{
	
	vec4 albedo_spec = texture(albedo_spec_tex, tex_coords.xy);
	vec4 frag_pos = texture(position_tex, tex_coords.xy);
	vec4 frag_normal = texture(normal_tex, tex_coords.xy);
	vec4 orms = texture(orms_tex, tex_coords.xy);
	vec4 specular = texture(spec_tex, tex_coords.xy);
	vec4 emissive = texture(emissive_tex, tex_coords.xy);
	float shadow_value = 1.0f;
	float shadow_found = 0.0;
	vec3 shadowed_light = vec3(0);
	for (int i = 0; i < num_shadow_maps; i++)
	{
        vec4 light_space_pos = light_space_matrix[0] * vec4(frag_pos.xyz, 1.0);
        vec3 proj_coords = light_space_pos.xyz / light_space_pos.w;
        proj_coords = proj_coords * 0.5 + 0.5;
        
        // Sample shadow map at correct coordinates
        shadow_value = texture(shadow_maps, vec3(proj_coords.xy, 0)).r;
		if (shadow_value < 0.5f)
		{
			shadow_value = 0.0f;
		}
		shadowed_light = light_colors[0].rgb * shadow_value;
		//shadow_value = pow(shadow_value, 100);
	}

	float roughness = orms.y;
	float metallic = orms.z;
	float ao = orms.x;
	float shininess_multiplier = orms.w;
	float shininess =  metallic * shininess_multiplier * material_shininess;

	float attenuation = 0;
	vec4 frag_color = vec4(vec3(0) + shadowed_light, albedo_spec.w);
	if (frag_pos.w < 0.5f)
	{
		//glClearColor(0.45f, 0.55f, 0.75f, 0.0f);
		FragColor = vec4(0.45f, 0.55f, 0.75f, 1.0f);
		return;
	}
	float last_intensity = 0;
	vec3 last_color = vec3(0);
	frag_color.xyz += albedo_spec.xyz * light_ambient.xyz;
	for (int i = 0; i < num_pointlights; i++)
	{ 

		vec3 light_position = point_lights[i].position.xyz;


		vec3 light_direction = normalize(point_lights[i].position.xyz - frag_pos.xyz);
		float light_distance = length(point_lights[i].position.xyz - frag_pos.xyz);
		vec3 normalized_normal = normalize(frag_normal.xyz);
		float lower_part = max(point_lights[i].attenuation.x  + point_lights[i].attenuation.y * light_distance  + point_lights[i].attenuation.z * pow(light_distance, 2), 0.1);
		attenuation = ( point_lights[i].color.w) / lower_part;
		float diffuse_intensity = dot(normalized_normal, light_direction);
		if (diffuse_intensity > 0 && attenuation > 0.025)
		{
			// diffuse
			vec3 diffuse_part = diffuse_intensity * point_lights[i].color.xyz * albedo_spec.xyz * ao;
			
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
			frag_color += vec4(total_brightness * attenuation * diffuse_intensity * point_lights[i].color.xyz * specular.rgb * metallic, 0.0) * frag_pos.w;
			last_intensity = point_lights[i].color.w;
			last_color = point_lights[i].color.xyz;
		}
		
	}
	frag_color *= vec4(albedo_spec.rgb, 1);
	frag_color.rgb += emissive.rgb * emissive.w;
	//frag_color -= vec4(vec3(0.25f) * (metallic * metallic), 0);
	FragColor = clamp(frag_color * shadow_value, 0.0, 1.0);
	//FragColor = specular;
	//FragColor = vec4(last_color, 1);
	//FragColor = vec4(point_lights[0].color.xyz, 1);
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
//	vec4 orms = texture(orms_tex, tex_coords.xy);
//
//	float roughness = orms.y;
//	float metallic = orms.z;
//	float ao = orms.x;
//	float shininess_multiplier = orms.w;
//	float shininess =  metallic * shininess_multiplier * material_shininess;
//
//	float attenuation = 0;
//	vec4 frag_color = vec4(vec3(0), albedo_spec.w);
//	if (frag_pos.w < 0.5f)
//	{
//		//glClearColor(0.45f, 0.55f, 0.75f, 0.0f);
//		FragColor = vec4(0.45f, 0.55f, 0.75f, 1.0f);
//		return;
//	}
//	frag_color.xyz += albedo_spec.xyz * light_ambient.xyz;
//	for (int i = 0; i < num_lights; i++)
//	{ 
//		vec3 light_direction = normalize(light_positions[i] - frag_pos.xyz);
//		float light_distance = length(light_positions[i] - frag_pos.xyz);
//		vec3 normalized_normal = normalize(frag_normal.xyz);
//
//		attenuation = ( light_intensities[i]) / (  (light_attenuations[i].x  + light_attenuations[i].y * light_distance  + light_attenuations[i].z * pow(light_distance, 2)));
//		float diffuse_intensity = dot(normalized_normal, light_direction);
//		if (diffuse_intensity > 0 && attenuation > 0.025)
//		{
//			// diffuse
//			vec3 diffuse_part = diffuse_intensity * light_colors[i].xyz * albedo_spec.xyz * ao;
//			
//			frag_color += vec4(attenuation * diffuse_part, 0.0);
//
//			// specular
//			float mat_shininess = (1 - roughness) * max(shininess, 1);
//			vec3 vector_to_camera = normalize(camera_position - frag_pos.xyz);
//			vec3 half_vector = normalize(light_direction + vector_to_camera);
//			float initial_brightness = max(dot(half_vector, normalized_normal), 0.0);
//
//			// calculate total brightness, and normalize it a bit through absorption (if it's not very shiny), and increase power if it's very shiny
//			float total_brightness = pow(initial_brightness, mat_shininess);
//			float area_normalization = (mat_shininess + 3)*(mat_shininess + 6 ) / ((16 * PI )*(2-(mat_shininess/2) + mat_shininess));
//			float light_absorption = metallic * clamp(mat_shininess / 32.0, 0.0, 1.0);
//			total_brightness *= light_absorption * area_normalization * roughness;
//			frag_color += vec4(total_brightness * attenuation * diffuse_intensity * light_colors[i].xyz * albedo_spec.xyz, 0.0) * frag_pos.w;
//		}
//		
//	}
//	frag_color *= albedo_spec;
//	//frag_color -= vec4(vec3(0.25f) * (metallic * metallic), 0);
//	FragColor = clamp(frag_color, 0.0, 1.0);
//	//FragColor = vec4(orm.yyy, 1);
//	//FragColor = vec4(attenuation.xxx, 1.0);
//	//FragColor = vec4(light_colors[1].xyz, 1.0);
//	//}
//}