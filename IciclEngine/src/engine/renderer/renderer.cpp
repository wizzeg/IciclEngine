#include <engine/renderer/renderer.h>
#include <engine/utilities/macros.h>
//#include <glm/ext/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>
#include <engine/game/components.h>

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
void Renderer::temp_render(RenderContext a_render_context)
{
	auto& mats = a_render_context.materials;
	auto& reqs = a_render_context.render_requests;

	uint64_t bound_mat = 0;
	GLuint bound_vao = 0;

	//GLuint current_gl_program = 0;

	size_t req_index = 0;
	size_t req_start_index = 0;
	GLuint tex_num = 0;

	// perhaps bind the first mat and mesh

	for (auto& mat : mats)
	{
		req_index = req_start_index;
		for (; req_index < reqs.size(); req_index++)
		{
			auto& req = reqs[req_index];
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
					for (auto& uniform : mat.uniforms)
					{
						if (uniform.type == typeid(std::string)) // this means it's texture
						{
							glActiveTexture((GLenum)(GL_TEXTURE0));
							glBindTexture(GL_TEXTURE_2D, uniform.texture_id);
							set_vec1i(1, "has_texture");
							break;
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
				//if (auto shader = shader_program.lock())
				//{
				//	shader->bind();
				//	shader->bind_uniform(typeid(glm::mat4), "proj", static_cast<void*>(glm::value_ptr(proj)));
				//	shader->bind_uniform(typeid(glm::mat4), "view", static_cast<void*>(glm::value_ptr(view)));
				//	shader->bind_uniform(typeid(glm::mat4), "model", static_cast<void*>(glm::value_ptr(req.model_matrix)));
				//	for (auto& uniform : mat.uniforms)
				//	{
				//		if (uniform.type == typeid(std::string)) // this means it's texture
				//		{
				//			glActiveTexture((GLenum)(GL_TEXTURE0));
				//			glBindTexture(GL_TEXTURE_2D, uniform.texture_id);
				//			shader->set_vec1i(1, "has_texture");
				//			break;
				//		}
				//	}
				//}
				//else
				//{
				//	PRINTLN("something went really wrong");
				//}
				//glBindVertexArray(req.vao);
				//bind_uniform(typeid(glm::mat4), "model", static_cast<UniformValue>(req.model_matrix));
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
				PRINTLN("material for the render request doesn't exist, or missorted");
				req_start_index = req_index;
				break;
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

void Renderer::set_proj_view_matrix(glm::mat4 a_proj, glm::mat4 a_view)
{
	proj = a_proj;
	view = a_view;
}

void Renderer::bind_uniform(std::type_index a_type, const std::string& a_location, UniformValue a_value) // good enough, lowers complexity
{
	// instead do an immediate cast and have this by a type T thing for immediate call
	//if (a_type == typeid(glm::vec3))
	//{
	//	set_vec3f(std::get<const float*>(a_value), a_location.c_str());
	//}
	//else if (a_type == typeid(glm::vec4))
	//{
	//	set_vec4f(*std::get<const float*>(a_value), a_location.c_str());
	//}
	if (a_type == typeid(int))
	{
		set_vec1i(std::get<int>(a_value), a_location.c_str());
	}
	else if (a_type == typeid(glm::mat4))
	{
		set_mat4fv(std::get<glm::mat4>(a_value), a_location.c_str());
	}
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