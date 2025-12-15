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

void Renderer::temp_render(RenderRequest& a_render_request)
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
			shader->set_vec1i((int)has_texture, "has_texture");
			shader->set_mat4fv(proj, "proj");
			shader->set_mat4fv(view, "view");
			shader->set_mat4fv(a_render_request.model_matrix, "model");
			glBindVertexArray(a_render_request.vao);
			glDrawElements(GL_TRIANGLES, a_render_request.indices_size, GL_UNSIGNED_INT, 0);
			if ((count % 50 == 0) || true)
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