#include <engine/renderer/renderer.h>
#include <engine/utilities/macros.h>
//#include <glm/ext/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>
#include <engine/game/components.h>
#include <engine/renderer/shader_loader.h>
#include <algorithm>

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
				if (bound_mat != mat.hash)
				{
					if (current_gl_program != mat.gl_program)
					{
						// bind program
						current_gl_program = mat.gl_program;
						glUseProgram(mat.gl_program);
						set_mat4fv(proj, "proj");
						set_mat4fv(view, "view");
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
				set_mat4fv(req.model_matrix, "model");
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

void Renderer::deffered_render(RenderContext& a_render_context, const DefferedBuffer& a_deffered_buffers)
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
		if (!mat.is_deffered)
		{
			continue;
		}
		for (; req_index < reqs.size(); req_index++)
		{
			RenderReq& req = reqs[req_index];
			if (req.mat_hash == mat.hash)
			{
				if (bound_mat != mat.hash)
				{
					if (current_gl_program != mat.gl_program)
					{
						// bind program
						current_gl_program = mat.gl_program;
						glUseProgram(mat.gl_program);
						set_mat4fv(proj, "proj");
						set_mat4fv(view, "view");
					}
					// bind mat
					bound_mat = mat.hash;
					int tex_index = 0;
					for (RuntimeUniform& uniform : mat.uniforms)
					{
						if (uniform.type == typeid(std::string)) // this means it's texture
						{
							glActiveTexture((GLenum)(GL_TEXTURE0 + tex_index));
							glBindTexture(GL_TEXTURE_2D, uniform.texture_id);
							set_vec1i(tex_index, uniform.location.c_str());
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
				set_mat4fv(req.model_matrix, "model");
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
				break;
			}
		}
	}

	if (lighting_program  == 0 || lighting_quad_vao == 0)
	{
		PRINTLN("lighting has not been initialized");
	}
	else
	{
		current_gl_program = lighting_program;
		glUseProgram(current_gl_program);
		a_deffered_buffers.output->bind();
		a_deffered_buffers.output->clear();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, a_deffered_buffers.gbuffer->get_position_texture());
		set_vec1i(0, "position_tex");

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, a_deffered_buffers.gbuffer->get_normal_texture());
		set_vec1i(1, "normal_tex");

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, a_deffered_buffers.gbuffer->get_albedo_spec_texture());
		set_vec1i(2, "albedo_spec_tex");
	
		std::vector<glm::vec3> light_poses; //{ glm::vec3(0, 0, 0), glm::vec3(10, 10, 10), glm::vec3(-10, -10, -10) };
		std::vector<glm::vec3> light_colors; //{ glm::vec3(0, 1, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, 1) };
		std::vector<glm::vec1> light_intensicies; //{glm::vec1(1), glm::vec1(1), glm::vec1(1)};
		GLsizei num_lights = (GLsizei)a_render_context.lights.size(); // 3;

		for (size_t i = 0; i < std::min((size_t)a_render_context.lights.size(), (size_t)256); i++)
		{
			light_poses.emplace_back(glm::vec3(a_render_context.lights[i].model_matrix[3]));
			//light_poses.emplace_back(glm::vec3(0));
		}
		for (size_t i = 0; i < std::min((size_t)a_render_context.lights.size(), (size_t)256); i++)
		{
			light_colors.emplace_back(a_render_context.lights[i].color);
		}
		for (size_t i = 0; i < std::min((size_t)a_render_context.lights.size(), (size_t)256); i++)
		{
			light_intensicies.emplace_back(a_render_context.lights[i].intensity);
		}
		if (num_lights > 0)
		{
			set_vec3fv(light_poses, num_lights, "light_positions");
			set_vec3fv(light_colors, num_lights, "light_colors");
			set_vec1fv(light_intensicies, num_lights, "light_intensities");
		}
		set_vec1i(num_lights, "num_lights");

		set_vec3f(static_cast<const float*>(&camera_position.x), "camera_position");
		render_lighting_quad(a_deffered_buffers.output->get_color_texture());
		a_deffered_buffers.output->unbind();
	}
	

	glActiveTexture((GLenum)(GL_TEXTURE0));
	glBindTexture(GL_TEXTURE_2D, 0);
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
	ShaderData shader = ShaderLoader::load_shader_from_path(lighting_shader_path);
	if (shader.loading_status == ShaderLoadedPath)
	{
		lighting_program = ShaderLoader::compile_shader(shader);
	}
	else
	{
		PRINTLN("failed loading lighting shader");
	}
	generate_lighting_quad();
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
		set_mat4fv(std::get<glm::mat4>(a_value), a_location.c_str());
	}
	else if (a_type == typeid(glm::ivec1))
	{
		set_vec1i(std::get<glm::ivec1>(a_value).x, a_location.c_str());
	}
	else if (a_type == typeid(std::string))
	{
		// we'd do something..
	}
}

void Renderer::render_lighting_quad(GLuint a_output_tex)
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

	// Position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

	// Texture coordinate attribute
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	glBindVertexArray(0);
}

void Renderer::generate_lighting_shader()
{

}

void Renderer::set_vec1f(const float a_value, const char* a_location) const
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
	glUniform1i(glGetUniformLocation(current_gl_program, a_location), a_value);
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
void Renderer::set_mat4fv(const glm::mat4 a_value, const char* a_location) const
{
	glUniformMatrix4fv(glGetUniformLocation(current_gl_program, a_location), 1, GL_FALSE, glm::value_ptr(a_value));
}