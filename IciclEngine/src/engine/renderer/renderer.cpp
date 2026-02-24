#include <engine/renderer/renderer.h>
#include <engine/utilities/macros.h>
//#include <glm/ext/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>
#include <engine/game/components.h>
#include <engine/renderer/shader_loader.h>
#include <algorithm>
constexpr auto MAX_POINT_LIGHTS = 512;
constexpr auto MAX_MODEL_INSTANCES = 4096; // 1024 and 512 makes weird bug with instanced renders, at 25/26k entities
constexpr auto HALF_MODEL_INSTANCES = 2048;

void Renderer::temp_render(MeshData& a_mesh, TransformDynamicComponent& a_world_pos)
{
	if (a_mesh.vao_load_status == ELoadStatus::Loaded && a_mesh.VAO != 0)
	{
		if (auto shader = shader_program.lock())
		{
			shader->bind();
			// Rotate 45 degrees per second around Y axis
			rotation += glm::radians(0.07f);

			// identity matrix first
			glm::mat4 model = glm::mat4(1.0f);

			// apply rotation around Y axis
			model = glm::translate(model, a_world_pos.position);

			model = glm::rotate(model, rotation, glm::vec3(0, 1, 0));
			shader->set_mat4fv(model, "model");
			glBindVertexArray(a_mesh.VAO);
			glDrawElements(GL_TRIANGLES, a_mesh.num_indicies, GL_UNSIGNED_INT, 0);
			shader->unbind();
		}

	}
}

void Renderer::temp_render(RenderRequest& a_render_request, glm::vec3 a_camera_position)
{
	if (a_render_request.vao != 0)
	{
		if (auto shader = shader_program.lock())
		{
			if (not_bound)
			{
				shader->bind();
				not_bound = false;
			}

			rotation += glm::radians(0.07f);
			//glm::mat4 temp = glm::rotate(a_render_request.model_matrix, rotation, glm::vec3(0, 1, 0));
			bool has_texture = (a_render_request.material_id != 0);
			if (has_texture)
			{
				glActiveTexture((GLenum)(GL_TEXTURE0));
				glBindTexture(GL_TEXTURE_2D, a_render_request.material_id);
			}
			//shader->set_vec3f(glm::value_ptr(a_camera_position), "camera_position");

			// set texture should have it's own thing, 
			// the "random" slot number just has to be sent to the vec1i uniform to the sampler
			// the random will just be in order that they are in the vector

			// so I just need to store a path so I can load the texture
			// then I also just need a string for the location (the uniform name for the sampler)

			// so store those two things in a UniformSampler as the type, that is a struct
			// then bind_uniform has it's way to handle that UniformSampler type

			// so the textures are simply uniforms

			shader->bind_uniform(typeid(glm::vec3), "camera_position", static_cast<void*>(glm::value_ptr(camera_position)));
			shader->set_vec1i((int)has_texture, "has_texture");
			shader->bind_uniform(typeid(glm::mat4), "proj", static_cast<void*>(glm::value_ptr(proj)));
			shader->bind_uniform(typeid(glm::mat4), "view", static_cast<void*>(glm::value_ptr(view)));
			shader->bind_uniform(typeid(glm::mat4), "model", static_cast<void*>(glm::value_ptr(a_render_request.model_matrix)));
			//shader->set_mat4fv(proj, "proj");
			//shader->set_mat4fv(view, "view");
			//shader->set_mat4fv(a_render_request.model_matrix, "model");
			glBindVertexArray(a_render_request.vao);
			glDrawElements(GL_TRIANGLES, a_render_request.indices_size, GL_UNSIGNED_INT, 0);
			if ((count % 1000 == 0))
			{
				shader->unbind();
				not_bound = true;
			}
			if (count == 1000000000)
			{
				count = 0;
			}
			else count++;
		}

	}
}
void Renderer::temp_render(RenderContext& a_render_context)
{
	auto& mats = a_render_context.materials;
	auto& reqs = a_render_context.render_requests;

	uint64_t bound_mat = 0;
	GLuint bound_vao = 0;

	size_t req_index = 0;
	size_t req_start_index = 0;
	GLuint tex_num = 0;

	// perhaps bind the first mat and mesh

	for (auto& mat : mats)
	{
		req_index = req_start_index;
		if (mat.is_deffered)
		{
			continue;
		}
		for (; req_index < reqs.size(); req_index++)
		{
			RenderReq& req = reqs[req_index];
			if (req.mat_hash == mat.hash)
			{
				// here do a check if the new req is not instanced, and if the instance batch > 0
				// if it is, issue the previous batch before going futher.
				if (bound_mat != mat.hash)
				{
					if (current_gl_program != mat.gl_program)
					{
						// bind program
						current_gl_program = mat.gl_program;
						glUseProgram(mat.gl_program);
						set_mat4fv(proj, 1, "proj");
						set_mat4fv(view, 1, "view");
					}
					// bind mat
					bound_mat = mat.hash;
					for (RuntimeUniform& uniform : mat.uniforms)
					{
						if (uniform.type == typeid(std::string)) // this means it's texture
						{
							glActiveTexture((GLenum)(GL_TEXTURE0));
							glBindTexture(GL_TEXTURE_2D, uniform.texture_id);
							set_vec1i(1, "has_texture");
							break;
						}
						else
						{
							bind_uniform(uniform.type, uniform.location, uniform.value);
						}
					}
				}
				if (bound_vao != req.vao)
				{
					// bind vao
					bound_vao = req.vao;
					glBindVertexArray(req.vao);
				}
				//draw
				set_mat4fv(req.model_matrix, 1, "model");
				glDrawElements(GL_TRIANGLES, req.indices_size, GL_UNSIGNED_INT, 0);

			}
			else if (req.mat_hash > mat.hash)
			{
				req_start_index = req_index;
				break;
			}
			else
			{
				//PRINTLN("material for the render request doesn't exist, or missorted");
				req_start_index = req_index;
				continue;
			}
		}
	}
	glActiveTexture((GLenum)(GL_TEXTURE0));
	glBindTexture(GL_TEXTURE_2D, 0);
	current_gl_program = 0;
	glUseProgram(current_gl_program);
}
void Renderer::temp_set_shader(std::weak_ptr<ShaderProgram> a_shader)
{
	shader_program = a_shader;
	shader_program.lock()->bind();
}

void Renderer::deffered_render(const RenderContext& a_render_context, const DefferedBuffer& a_deffered_buffers)
{

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	// do shadow passes here for each light that needs shadowing
	const FrameBuffer* gbuffer = a_deffered_buffers.gbuffer;
	const FrameBuffer* output = a_deffered_buffers.output;
	if (!gbuffer) return;
	deffered_geometry_pass(a_render_context, gbuffer);

	if (lighting_program == 0 || lighting_quad_vao == 0)
	{
		PRINTLN("lighting has not been initialized");
	}
	else
	{

		// if I want post procesing effects, like bloom or something for emissive, I'd do it here or after
		////////////////////////////////////////////////////////////////////////////////
		// lighting pass



		current_gl_program = lighting_program;
		glUseProgram(current_gl_program);
		output->bind();
		output->clear();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gbuffer->get_position_texture());
		set_vec1i(0, "position_tex");

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gbuffer->get_normal_texture());
		set_vec1i(1, "normal_tex");

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gbuffer->get_albedo_spec_texture());
		set_vec1i(2, "albedo_spec_tex");

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, gbuffer->get_orms_texture());
		set_vec1i(3, "orms_tex");

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, gbuffer->get_spec_texture());
		set_vec1i(4, "spec_tex");

		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, gbuffer->get_emissive_texture());
		set_vec1i(5, "emissive_tex");

		if (auto* shadow_maps = a_deffered_buffers.shadow_maps)
		{
			GLuint shadow_map_array = shadow_maps->get_shadow_maps_texture_array();
			glActiveTexture(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_2D_ARRAY, shadow_map_array); 
			set_vec1i(6, "shadow_maps");
		}


		update_pointlight_SSBO(a_render_context.lights);

		GLsizei num_lights = (GLsizei)a_render_context.lights.size();

		//set_vec1i((int)light_matrices.size(), "num_shadow_maps");
		set_vec1i(num_lights, "num_lights");

		set_vec3f(static_cast<const float*>(&camera_position.x), "camera_position");
		render_lighting_quad();

		deffered_ui_pass(a_render_context, output);

		output->unbind();
		/// lighting pass end
		//////////////////////////////////////////////////////////////////////////////////////////////////

		// UI pass // would also put transparent before this
		//////////////////////////////////////////////////////////////////////////////////////////////////
		
		//std::string order = "";
		//for (char i = 32; i < (95 + 32); i++)
		//{
		//	//PRINTLN("{}", i);
		//	order += "'";
		//	order += i;
		//	order += "' ";
		//}
		//PRINTLN("letter order: {}", order);
	}

	current_gl_program = 0;
	glUseProgram(current_gl_program);
}

void Renderer::set_proj_view_matrix(glm::mat4 a_proj, glm::mat4 a_view)
{
	proj = a_proj;
	view = a_view;
}

void Renderer::set_lighting_shader(const std::string& a_shader_path)
{
	lighting_shader_path = a_shader_path;
}

void Renderer::initialize()
{
	ShaderData lighting_shader = ShaderLoader::load_shader_from_path(lighting_shader_path);
	
	if (lighting_shader.loading_status == ShaderLoadedPath)
	{
		lighting_program = ShaderLoader::compile_shader(lighting_shader);
	}
	else
	{
		PRINTLN("failed loading lighting shader");
	}
	ShaderData ui_shader = ShaderLoader::load_shader_from_path(ui_shader_path);
	if (ui_shader.loading_status == ShaderLoadedPath)
	{
		ui_program = ShaderLoader::compile_shader(ui_shader);
		if (ui_program != 0)
		{
			glUseProgram(ui_program);
			// no I have to actually load it through my asset loader...
			// I guess just do it as soon as engine context is done.
		}
	}
	else
	{
		PRINTLN("failed loading ui shader");
	}
	generate_lighting_quad();
	create_pointlight_SSBO();
	create_instance_SSBO();
}

void Renderer::bind_uniform(std::type_index a_type, const std::string& a_location, UniformValue a_value) // good enough, lowers complexity
{
	// instead do an immediate cast and have this by a type T thing for immediate call
	if (a_type == typeid(glm::vec3))
	{
		set_vec3f(reinterpret_cast<const float*>(&std::get<glm::vec3>(a_value)), a_location.c_str());
	}
	else if (a_type == typeid(glm::vec4))
	{
		set_vec4f(reinterpret_cast<const float*>(&std::get<glm::vec4>(a_value)), a_location.c_str());
	}
	else if (a_type == typeid(int))
	{
		set_vec1i(std::get<int>(a_value), a_location.c_str());
	}
	else if (a_type == typeid(glm::mat4))
	{
		set_mat4fv(std::get<glm::mat4>(a_value), 1, a_location.c_str());
	}
	else if (a_type == typeid(glm::ivec1))
	{
		set_vec1i(std::get<glm::ivec1>(a_value).x, a_location.c_str());
	}
	else if (a_type == typeid(float))
	{
		set_vec1f(std::get<float>(a_value), a_location.c_str());
	}
	else if (a_type == typeid(std::string))
	{
		// we'd do something..
	}
}

void Renderer::render_lighting_quad()
{
	generate_lighting_quad();
	glBindVertexArray(lighting_quad_vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void Renderer::generate_lighting_quad()
{
	if (lighting_quad_vao != 0) return;  // Already created

	// Quad vertices in NDC (Normalized Device Coordinates: -1 to 1)
	// Position (x, y, z) + TexCoords (u, v)
	float quadVertices[] = {
		// Positions        // Texture Coords
		-1.0f,  1.0f, 0.0f,  0.0f, 1.0f,  // Top-left
		-1.0f, -1.0f, 0.0f,  0.0f, 0.0f,  // Bottom-left
		 1.0f, -1.0f, 0.0f,  1.0f, 0.0f,  // Bottom-right

		-1.0f,  1.0f, 0.0f,  0.0f, 1.0f,  // Top-left
		 1.0f, -1.0f, 0.0f,  1.0f, 0.0f,  // Bottom-right
		 1.0f,  1.0f, 0.0f,  1.0f, 1.0f   // Top-right
	};

	glGenVertexArrays(1, &lighting_quad_vao);
	glGenBuffers(1, &lighting_quad_vbo);

	glBindVertexArray(lighting_quad_vao);
	glBindBuffer(GL_ARRAY_BUFFER, lighting_quad_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	// pos
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

	// tex coords
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	glBindVertexArray(0);
}

void Renderer::generate_lighting_shader()
{

}

// could quite easily make these dynamically, just make a request for an int and you get the slot.. but binding stuff to it is tricky
void Renderer::create_pointlight_SSBO()
{
	if (pointlight_ssbo != 0) return;
	// make the SSBO
	glGenBuffers(1, &pointlight_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointlight_ssbo);

	// allocate the gpu space for the num_lights + buffer
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 4 + sizeof(PointLightSSBO) * MAX_POINT_LIGHTS, nullptr, GL_DYNAMIC_DRAW);

	// binding to binding point 0 (for now, probably always)
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, pointlight_ssbo);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Renderer::update_pointlight_SSBO(const std::vector<PointLightSSBO>& a_point_lights) // maybe make this generic somehow
{
	if (pointlight_ssbo == 0) return;

	// bind the ssbo
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointlight_ssbo);

	// orphan buffer
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		sizeof(int) * 4 + sizeof(PointLightSSBO) * MAX_POINT_LIGHTS, nullptr, GL_DYNAMIC_DRAW);

	// set num lights
	int num_lights = std::min(static_cast<int>(a_point_lights.size()), MAX_POINT_LIGHTS);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &num_lights);

	// check if we can upload data and update
	if (num_lights > 0) {
		if (a_point_lights.size() <= MAX_POINT_LIGHTS)
		{


			glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 4,
				sizeof(PointLightSSBO) * num_lights, a_point_lights.data());
		}
		else
		{
			std::vector<PointLightSSBO> new_ssbo(MAX_POINT_LIGHTS);
			std::memcpy(new_ssbo.data(), a_point_lights.data(),
				MAX_POINT_LIGHTS * sizeof(PointLightSSBO));
			int new_num_lights = static_cast<int>(new_ssbo.size());

			glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 4,
				sizeof(PointLightSSBO) * new_num_lights, new_ssbo.data());
			//PRINTLN("too many lights, does not fit allocation");
		}

	}
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Renderer::create_shadow_framebuffers()
{
	if (shadow_maps != nullptr) return;

	glfw_context->create_framebuffer("shadow_map_array", 2048, 2048, EFramebufferType::ShadowMapArray);
	shadow_maps = glfw_context->get_framebuffer("shadow_map_array");
}

void Renderer::load_ui_shader()
{

}

void Renderer::set_shadow_maps(FrameBuffer* a_shadow_maps)
{
	shadow_maps = a_shadow_maps;
};
void Renderer::create_instance_SSBO()
{
	if (model_instance_ssbo == 0)
	{
		// make the SSBO
		glGenBuffers(1, &model_instance_ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, model_instance_ssbo);

		// allocate the gpu space for the num_lights + buffer
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 4 + sizeof(glm::mat4) * MAX_MODEL_INSTANCES, nullptr, GL_DYNAMIC_DRAW);

		// binding to binding point 0 (for now, probably always)
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, model_instance_ssbo);
		int half = HALF_MODEL_INSTANCES;
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &half);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}


	if (ui_instance_ssbo == 0)
	{
		// make the SSBO
		glGenBuffers(1, &ui_instance_ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ui_instance_ssbo);

		// allocate the gpu space for the num_lights + buffer
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 4 + sizeof(UISSBO) * MAX_MODEL_INSTANCES, nullptr, GL_DYNAMIC_DRAW);

		// binding to binding point 0 (for now, probably always)
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ui_instance_ssbo);
		int half = HALF_MODEL_INSTANCES;
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &half);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
	
}

void Renderer::update_insance_SSBO(const std::vector<glm::mat4>& a_model_matrices)
{
	if (model_instance_ssbo == 0) return;
	int half_models = HALF_MODEL_INSTANCES;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, model_instance_ssbo);
	// write to first half

	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 4 + sizeof(glm::mat4) * MAX_MODEL_INSTANCES, nullptr, GL_DYNAMIC_DRAW);
		
	//if (fences[(size_t)instance_half] != nullptr)
	//{
	//	glClientWaitSync(fences[(size_t)instance_half], 0, GL_TIMEOUT_IGNORED);
	//	glDeleteSync(fences[(size_t)instance_half]);
	//	fences[(size_t)instance_half] = nullptr;
	//}

	std::vector<glm::mat4> model_matrices = a_model_matrices;
	if (model_matrices.size() > HALF_MODEL_INSTANCES)
	{
		//std::vector<PointLightSSBO> new_model(MAX_POINT_LIGHTS);
		//std::memcpy(new_model.data(), a_model_matrices.data(),
		//	MAX_POINT_LIGHTS * sizeof(PointLightSSBO));
		PRINTLN("too many in instance draw");
		return;
	}
	int num_models = std::min(static_cast<int>(model_matrices.size()), HALF_MODEL_INSTANCES);
	if (!instance_half)
	{
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &half_models);

		glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 4,
			sizeof(glm::mat4) * num_models, model_matrices.data());
		instance_half = true;
		set_vec1i(1, "instance_buffer");
	}
	else
	{
		// always to first int, now I'm storing the "HALF_MODEL_INSTANCES" to get correct offset
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &half_models);

		glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 4 + HALF_MODEL_INSTANCES * sizeof(glm::mat4),
			sizeof(glm::mat4) * num_models, model_matrices.data());
		instance_half = false;
		set_vec1i(2, "instance_buffer");
	}
	//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Renderer::update_ui_insance_SSBO(const std::vector<UISSBO>& ui_SSBO)
{
	if (ui_instance_ssbo == 0) return;
	int half_models = HALF_MODEL_INSTANCES;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ui_instance_ssbo);
	// write to first half
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 4 + sizeof(ui_SSBO) * MAX_MODEL_INSTANCES, nullptr, GL_DYNAMIC_DRAW);

	if (ui_SSBO.size() > HALF_MODEL_INSTANCES)
	{
		//std::vector<PointLightSSBO> new_model(MAX_POINT_LIGHTS);
		//std::memcpy(new_model.data(), a_model_matrices.data(),
		//	MAX_POINT_LIGHTS * sizeof(PointLightSSBO));
		PRINTLN("too many in instance draw");
		return;
	}
	int num_models = std::min(static_cast<int>(ui_SSBO.size()), HALF_MODEL_INSTANCES);
	if (!ui_instance_half)
	{
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &half_models);

		glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 4,
			sizeof(UISSBO) * num_models, ui_SSBO.data());
		ui_instance_half = true;
		set_vec1i(1, "instance_buffer");
	}
	else
	{
		// always to first int, now I'm storing the "HALF_MODEL_INSTANCES" to get correct offset
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &half_models);

		glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 4 + HALF_MODEL_INSTANCES * sizeof(UISSBO),
			sizeof(UISSBO) * num_models, ui_SSBO.data());
		ui_instance_half = false;
		set_vec1i(2, "instance_buffer");
	}
}

void Renderer::deffered_geometry_pass(const RenderContext& a_render_context, const FrameBuffer* g_buffer)
{
	const auto& mats = a_render_context.materials;
	const auto& reqs = a_render_context.render_requests;

	g_buffer->bind();
	g_buffer->clear();

	uint64_t bound_mat = 0;
	GLuint bound_vao = 0;

	size_t req_index = 0;
	size_t req_start_index = 0;
	GLuint tex_num = 0;

	// perhaps bind the first mat and mesh
	bool mipmaps_set = false;
	bool mipmap = true;
	bool set_mipmaps = false;
	bool mat_instance = false;

	std::vector<glm::mat4> instanced_models;
	instanced_models.reserve(HALF_MODEL_INSTANCES);
	bool started_instance_batch = false;
	bool issued_instance_batch = false;
	GLsizei batch_indices_count = 0;
	bool broke_batch = false;
	uint64_t batch_mat = hashed_string_64("");

	int draw_calls = 0;

	for (const RuntimeMaterial& mat : mats)
	{
		mipmaps_set = false;
		mat_instance = mat.instantiable;

		req_index = req_start_index;
		if (!mat.is_deffered)
		{
			continue;
		}
		for (; req_index < reqs.size(); req_index++)
		{
			const RenderReq& req = reqs[req_index];
			if (req.mat_hash == mat.hash)
			{
				broke_batch = (started_instance_batch && (!req.instanced || instanced_models.size() >= HALF_MODEL_INSTANCES))
					|| (started_instance_batch && (bound_vao != req.vao || instanced_models.size() >= HALF_MODEL_INSTANCES))
					|| (started_instance_batch && (req.mipmap != mipmap))
					|| (started_instance_batch && (req.mat_hash != batch_mat));

				if (broke_batch) //draw the instanced before drawing non instanced
				{
					// draw it before starting on new
					update_insance_SSBO(instanced_models);
					glDrawElementsInstanced(GL_TRIANGLES, batch_indices_count, GL_UNSIGNED_INT, 0, (GLsizei)instanced_models.size());
					
					//if (fences[(size_t)instance_half] != nullptr) {
					//	glDeleteSync(fences[(size_t)instance_half]);
					//}
					//fences[(size_t)instance_half] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

					started_instance_batch = false;
					issued_instance_batch = true;
					instanced_models.clear();
					set_vec1i(0, "instance_buffer");
					draw_calls++;
				}

				if (req.instanced && mat_instance)
				{
					instanced_models.push_back(req.model_matrix);
					started_instance_batch = true;
					issued_instance_batch = false;
					batch_mat = req.mat_hash;
					batch_indices_count = req.indices_size;
				}

				if (!mipmaps_set)
				{
					set_mipmaps = true;
					mipmap = req.mipmap;
					mipmaps_set = true;
				}
				else if (req.mipmap != mipmap)
				{
					set_mipmaps = true;
					mipmap = req.mipmap;
				}
				if (bound_mat != mat.hash || set_mipmaps)
				{
					if (current_gl_program != mat.gl_program)
					{
						// bind program
						current_gl_program = mat.gl_program;
						glUseProgram(mat.gl_program);
						set_mat4fv(proj, 1, "proj");
						set_mat4fv(view, 1, "view");
						//set_vec3f(reinterpret_cast<const float*>(&camera_position), "camera_world_pos");
					}
					// bind mat
					bound_mat = mat.hash;
					GLint tex_index = 0;
					for (const RuntimeUniform& uniform : mat.uniforms)
					{
						if (uniform.type == typeid(std::string)) // this means it's texture
						{
							GLint combined = (GL_TEXTURE0 + tex_index);
							glActiveTexture(combined);
							glBindTexture(GL_TEXTURE_2D, uniform.texture_id);
							if (set_mipmaps)
							{
								if (mipmap)
								{
									glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
								}
								else
								{
									glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
								}
							}
							set_vec1i(tex_index, uniform.location.c_str());
							tex_index++;
						}
						else
						{
							bind_uniform(uniform.type, uniform.location, uniform.value);
						}
					}
					set_mipmaps = false;
					set_vec1i(0, "instance_buffer");
				}
				if (bound_vao != req.vao)
				{
					// bind vao
					bound_vao = req.vao;
					glBindVertexArray(req.vao);
				}
				//draw
				if (!req.instanced || !mat.instantiable)
				{
					set_mat4fv(req.model_matrix, 1, "model");
					glDrawElements(GL_TRIANGLES, req.indices_size, GL_UNSIGNED_INT, 0);
					draw_calls++;
				}


			}
			//else if (req.mat_hash > mat.hash)
			//{
			//	req_start_index = req_index;
			//	break;
			//}
			else
			{
				//PRINTLN("material for the render request doesn't exist, or missorted");
				req_start_index = req_index;
				break;
			}
		}
	}

	/// check so that there's no lingering batch
	if (started_instance_batch && !issued_instance_batch)
	{
		update_insance_SSBO(instanced_models);
		glDrawElementsInstanced(GL_TRIANGLES, batch_indices_count, GL_UNSIGNED_INT, 0, (GLsizei)instanced_models.size());
		//if (fences[(size_t)instance_half] != nullptr) {
		//	glDeleteSync(fences[(size_t)instance_half]);
		//}
		//fences[(size_t)instance_half] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		started_instance_batch = false;
		issued_instance_batch = true;
		instanced_models.clear();
		set_vec1i(0, "instance_buffer");
		draw_calls++;
		instanced_models.clear();
	}
	//PRINTLN("number of draw calls: {}", draw_calls)
}

void Renderer::deffered_shadowmap_pass(const RenderContext& a_render_context, const FrameBuffer* shadow_array, size_t a_shadowmap_index)
{

	const auto& mats = a_render_context.materials;
	const auto& reqs = a_render_context.render_requests;

	glm::vec3 light_direction = glm::normalize(
		glm::mat3(a_render_context.shadow_lights[a_shadowmap_index].lightspace_matrix)
		* glm::vec3(0.0f, 0.0f, -1.0f));
	glm::mat4 light_space_matrix =
		calculate_directional_light_matrix_fitted(glm::radians(a_render_context.shadow_lights[a_shadowmap_index].rotation), view, proj);
	//light_space_matrix = proj * view;
	uint64_t bound_mat = 0;
	GLuint bound_vao = 0;

	size_t req_index = 0;
	size_t req_start_index = 0;
	GLuint tex_num = 0;

	// perhaps bind the first mat and mesh
	bool mat_instance = false;

	std::vector<glm::mat4> instanced_models;
	instanced_models.reserve(HALF_MODEL_INSTANCES);
	bool started_instance_batch = false;
	bool issued_instance_batch = false;
	GLsizei batch_indices_count = 0;
	bool broke_batch = false;
	uint64_t batch_mat = hashed_string_64("");

	int draw_calls = 0;

	// so we want to check if the material casts shadows.. if not, the skip
	// then do as normal, except we don't care about mipmaps
	// but always set all the uniforms that are required
	for (const RuntimeMaterial& mat : mats)
	{
		mat_instance = mat.instantiable;

		req_index = req_start_index;
		if (!mat.casts_shadows) // we do not care about if it's deffered actually
		{
			continue;
		}
		for (; req_index < reqs.size(); req_index++)
		{
			const RenderReq& req = reqs[req_index];
			if (req.mat_hash == mat.hash)
			{
				broke_batch = (started_instance_batch && (!req.instanced || instanced_models.size() >= HALF_MODEL_INSTANCES))
					|| (started_instance_batch && (bound_vao != req.vao || instanced_models.size() >= HALF_MODEL_INSTANCES))
					|| (started_instance_batch && (req.mat_hash != batch_mat));

				if (broke_batch) //draw the instanced before drawing non instanced
				{
					// draw it before starting on new
					update_insance_SSBO(instanced_models);
					glDrawElementsInstanced(GL_TRIANGLES, batch_indices_count, GL_UNSIGNED_INT, 0, (GLsizei)instanced_models.size());
					started_instance_batch = false;
					issued_instance_batch = true;
					instanced_models.clear();
					set_vec1i(0, "instance_buffer");
					draw_calls++;
				}

				if (req.instanced && mat_instance)
				{
					instanced_models.push_back(req.model_matrix);
					started_instance_batch = true;
					issued_instance_batch = false;
					batch_mat = req.mat_hash;
					batch_indices_count = req.indices_size;
				}
				if (bound_mat != mat.hash)
				{
					if (current_gl_program != mat.gl_program)
					{
						// bind program
						set_vec1i(0, "shadow_pass");
						current_gl_program = mat.gl_program;
						glUseProgram(mat.gl_program);
						set_mat4fv(proj, 1, "proj");
						set_mat4fv(view, 1, "view");
						set_mat4fv(light_space_matrix, 1, "light_space_matrix");
						set_vec1i(1, "shadow_pass");
						//set_vec3f(reinterpret_cast<const float*>(&camera_position), "camera_world_pos");
					}
					// bind mat
					bound_mat = mat.hash;
					GLint tex_index = 0;
					for (const RuntimeUniform& uniform : mat.uniforms)
					{
						if (uniform.type == typeid(std::string)) // this means it's texture
						{
							GLint combined = (GL_TEXTURE0 + tex_index);
							glActiveTexture(combined);
							glBindTexture(GL_TEXTURE_2D, uniform.texture_id);
							set_vec1i(tex_index, uniform.location.c_str());
							tex_index++;
						}
						else
						{
							bind_uniform(uniform.type, uniform.location, uniform.value);
						}
					}
					set_vec1i(0, "instance_buffer");
				}
				if (bound_vao != req.vao)
				{
					// bind vao
					bound_vao = req.vao;
					glBindVertexArray(req.vao);
				}
				//draw
				if (!req.instanced || !mat.instantiable)
				{
					set_mat4fv(req.model_matrix, 1, "model");
					glDrawElements(GL_TRIANGLES, req.indices_size, GL_UNSIGNED_INT, 0);
					draw_calls++;

				}


			}
			//else if (req.mat_hash > mat.hash)
			//{
			//	req_start_index = req_index;
			//	break;
			//}
			else
			{
				//PRINTLN("material for the render request doesn't exist, or missorted");
				req_start_index = req_index;
				break;
			}
		}
	}

	/// check so that there's no lingering batch
	if (started_instance_batch && !issued_instance_batch)
	{
		update_insance_SSBO(instanced_models);
		glDrawElementsInstanced(GL_TRIANGLES, batch_indices_count, GL_UNSIGNED_INT, 0, (GLsizei)instanced_models.size());
		draw_calls++;
		instanced_models.clear();
	}
	set_vec1i(0, "shadow_pass");
	glUseProgram(0);
	//PRINTLN("number of draw calls: {}", draw_calls)
}

void Renderer::deffered_ui_pass(const RenderContext& a_render_context, const FrameBuffer* a_output)
{

	const auto& mats = a_render_context.ui_materials;
	const auto& reqs = a_render_context.ui_render_requests;

	a_output->bind();
	glDisable(GL_DEPTH_TEST);
	uint64_t bound_mat = 0;
	GLuint bound_vao = 0;

	size_t req_index = 0;
	size_t req_start_index = 0;
	GLuint tex_num = 0;

	// perhaps bind the first mat and mesh
	bool mipmaps_set = false;
	bool mipmap = true;
	bool set_mipmaps = false;
	bool mat_instance = false;

	std::vector<UISSBO> ui_ssbo;
	ui_ssbo.reserve(HALF_MODEL_INSTANCES);
	bool started_instance_batch = false;
	bool issued_instance_batch = false;
	GLsizei batch_indices_count = 0;
	bool broke_batch = false;
	uint64_t batch_mat = hashed_string_64("");

	int draw_calls = 0;

	glBindVertexArray(lighting_quad_vao);
	for (const RuntimeMaterial& mat : mats)
	{
		req_index = req_start_index;
		for (; req_index < reqs.size(); req_index++)
		{
			const UIRenderRequest& req = reqs[req_index];
			if (req.material == mat.hash)
			{
				broke_batch = (started_instance_batch && ui_ssbo.size() >= HALF_MODEL_INSTANCES);

				if (broke_batch) //draw the instanced before drawing non instanced
				{
					// draw it before starting on new
					update_ui_insance_SSBO(ui_ssbo);
					glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(ui_ssbo.size()));
					started_instance_batch = false;
					issued_instance_batch = true;
					ui_ssbo.clear();
					set_vec1i(0, "instance_buffer");
					draw_calls++;
				}
				
				ui_ssbo.emplace_back(req.color, req.uv_offset, glm::vec2(0.1f, 0.1f), req.position, req.extents);
				started_instance_batch = true;
				issued_instance_batch = false;
				batch_mat = req.material;
				if (bound_mat != mat.hash || set_mipmaps)
				{
					if (current_gl_program != mat.gl_program)
					{
						// bind program
						current_gl_program = mat.gl_program;
						glUseProgram(mat.gl_program);
						set_mat4fv(proj, 1, "proj");
						set_mat4fv(view, 1, "view");
						//set_vec3f(reinterpret_cast<const float*>(&camera_position), "camera_world_pos");
					}
					// bind mat
					bound_mat = mat.hash;
					GLint tex_index = 0;
					for (const RuntimeUniform& uniform : mat.uniforms)
					{
						if (uniform.type == typeid(std::string)) // this means it's texture
						{
							GLint combined = (GL_TEXTURE0 + tex_index);
							glActiveTexture(combined);
							glBindTexture(GL_TEXTURE_2D, uniform.texture_id);
							if (set_mipmaps)
							{
								if (mipmap)
								{
									glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
								}
								else
								{
									glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
								}
							}
							set_vec1i(tex_index, uniform.location.c_str());
							tex_index++;
						}
						else
						{
							bind_uniform(uniform.type, uniform.location, uniform.value);
						}
					}
					set_mipmaps = false;
					set_vec1i(0, "instance_buffer");
				}
				//draw
				//if (!req.instanced || !mat.instantiable)
				//{
				//	set_mat4fv(req.model_matrix, 1, "model");
				//	glDrawElements(GL_TRIANGLES, req.indices_size, GL_UNSIGNED_INT, 0);
				//	draw_calls++;
				//}


			}
			//else if (req.mat_hash > mat.hash)
			//{
			//	req_start_index = req_index;
			//	break;
			//}
			else
			{
				//PRINTLN("material for the render request doesn't exist, or missorted");
				req_start_index = req_index;
				break;
			}
		}
		
	}

	/// check so that there's no lingering batch
	if (started_instance_batch && !issued_instance_batch)
	{
		update_ui_insance_SSBO(ui_ssbo);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(ui_ssbo.size()));
		started_instance_batch = false;
		issued_instance_batch = true;
		ui_ssbo.clear();
		set_vec1i(0, "instance_buffer");
		draw_calls++;
	}
	glEnable(GL_DEPTH_TEST);
	glUseProgram(0);
	glBindVertexArray(0);
	a_output->unbind();
}

void Renderer::set_vec1f(const float a_value, const char* a_location) const // perhaps later cache all the actual locations instead of string look up
{
	glUniform1f(glGetUniformLocation(current_gl_program, a_location), a_value);
}
void Renderer::set_vec2f(const float a_value[2], const char* a_location) const
{
	glUniform2f(glGetUniformLocation(current_gl_program, a_location), a_value[0], a_value[1]);
}
void Renderer::set_vec3f(const float a_value[3], const char* a_location) const
{
	glUniform3f(glGetUniformLocation(current_gl_program, a_location), a_value[0], a_value[1], a_value[2]);
}
void Renderer::set_vec4f(const float a_value[4], const char* a_location) const
{
	glUniform4f(glGetUniformLocation(current_gl_program, a_location), a_value[0], a_value[1], a_value[2], a_value[3]);
}
void Renderer::set_vec1fv(const std::vector<glm::vec1>& a_value, GLsizei a_count, const char* a_location) const
{
	glUniform1fv(glGetUniformLocation(current_gl_program, a_location), a_count, glm::value_ptr(a_value[0]));
}
void Renderer::set_vec2fv(const std::vector<glm::vec2>& a_value, GLsizei a_count, const char* a_location) const
{
	glUniform2fv(glGetUniformLocation(current_gl_program, a_location), a_count, glm::value_ptr(a_value[0]));
}
void Renderer::set_vec3fv(const std::vector<glm::vec3>& a_value, GLsizei a_count, const char* a_location) const
{
	glUniform3fv(glGetUniformLocation(current_gl_program, a_location), a_count, glm::value_ptr(a_value[0]));
}
void Renderer::set_vec4fv(const std::vector<glm::vec4>& a_value, GLsizei a_count, const char* a_location) const
{
	glUniform4fv(glGetUniformLocation(current_gl_program, a_location), a_count, glm::value_ptr(a_value[0]));
}
void Renderer::set_vec1i(const int a_value, const char* a_location) const
{
	auto loc = glGetUniformLocation(current_gl_program, a_location);
	glUniform1i(loc, a_value);
}
void Renderer::set_vec2i(const int a_value[2], const char* a_location) const
{
	glUniform2i(glGetUniformLocation(current_gl_program, a_location), a_value[0], a_value[1]);
}
void Renderer::set_vec3i(const int a_value[3], const char* a_location) const
{
	glUniform3i(glGetUniformLocation(current_gl_program, a_location), a_value[0], a_value[1], a_value[2]);
}
void Renderer::set_vec4i(const int a_value[4], const char* a_location) const
{
	glUniform4i(glGetUniformLocation(current_gl_program, a_location), a_value[0], a_value[1], a_value[2], a_value[3]);
}

void Renderer::set_vec1ui(const unsigned int a_value, const char* a_location) const
{
	glUniform1ui(glGetUniformLocation(current_gl_program, a_location), a_value);
}
void Renderer::set_vec2ui(const unsigned int a_value[2], const char* location) const
{
	glUniform2ui(glGetUniformLocation(current_gl_program, location), a_value[0], a_value[1]);
}
void Renderer::set_vec3ui(const unsigned int a_value[3], const char* a_location) const
{
	glUniform3ui(glGetUniformLocation(current_gl_program, a_location), a_value[0], a_value[1], a_value[2]);
}
void Renderer::set_vec4ui(const unsigned int a_value[4], const char* a_location) const
{
	glUniform4ui(glGetUniformLocation(current_gl_program, a_location), a_value[0], a_value[1], a_value[2], a_value[3]);
}
void Renderer::set_mat4fv(const glm::mat4 a_value, GLsizei a_count, const char* a_location) const
{
	glUniformMatrix4fv(glGetUniformLocation(current_gl_program, a_location), a_count, GL_FALSE, glm::value_ptr(a_value));
}