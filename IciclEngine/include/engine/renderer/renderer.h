#pragma once
#include <engine/renderer/render_info.h>
#include <engine/renderer/shader_program.h>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <memory>
#include <engine/game/components.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <string>
#include <typeindex>
#include <engine/resources/data_structs.h>
#include <engine/renderer/frame_buffer.h>
#include <engine/renderer/glfw_context.h>
#include <algorithm>

struct DefferedBuffer
{
	FrameBuffer* shadow_maps = nullptr;
	FrameBuffer* gbuffer = nullptr;
	FrameBuffer* output = nullptr;
};

struct ShadowBuffer
{
	glm::mat4 position;
	FrameBuffer* shadow_map;
};

struct Renderer
{
	Renderer(std::shared_ptr<GLFWContext> a_context) : glfw_context(a_context) {}
	~Renderer()
	{
		if (lighting_quad_vbo != 0)
		{
			glDeleteBuffers(1, &lighting_quad_vbo);
		}
		if (lighting_quad_vao != 0)
		{
			glDeleteBuffers(1, &lighting_quad_vao);

		}
	}
	float rotation = 0;
	std::weak_ptr<ShaderProgram> shader_program;
	void temp_render(MeshData& a_mesh, TransformDynamicComponent& a_world_pos);
	void temp_render(RenderRequest& a_render_request, glm::vec3 a_camera_positon = glm::vec3(0));
	void temp_render(RenderContext& a_render_context);
	void temp_set_shader(std::weak_ptr<ShaderProgram> a_shader);

	void deffered_render(const RenderContext& a_render_context, const DefferedBuffer& a_deffered_buffers);

	void set_camera_position(glm::vec3 a_position) { camera_position = a_position; };
	void set_proj_view_matrix(glm::mat4 a_proj, glm::mat4 a_view);
	void set_lighting_shader(const std::string& a_shader_path);
	void set_shadow_maps(FrameBuffer* a_shadow_maps);

	void initialize();
private:

    // Helper function
    std::vector<glm::vec4> get_frustum_corners_world_space(
        const glm::mat4& projection,
        const glm::mat4& view)
    {
        glm::mat4 inv = glm::inverse(projection * view);

        std::vector<glm::vec4> corners;
        for (unsigned int x = 0; x < 2; ++x) {
            for (unsigned int y = 0; y < 2; ++y) {
                for (unsigned int z = 0; z < 2; ++z) {
                    glm::vec4 corner = inv * glm::vec4(
                        2.0f * x - 1.0f,
                        2.0f * y - 1.0f,
                        2.0f * z - 1.0f,
                        1.0f
                    );
                    corners.push_back(corner / corner.w);
                }
            }
        }
        return corners;
    }
    glm::mat4 calculate_directional_light_matrix_fitted(
        const glm::vec3 light_direction,
        const glm::mat4 camera_view,
        const glm::mat4 camera_projection)
    {
        // Get camera frustum corners in world space
        std::vector<glm::vec4> frustum_corners = get_frustum_corners_world_space(
            camera_projection, camera_view
        );

        // Calculate center of frustum
        glm::vec3 center = glm::vec3(0.0f);
        for (const auto& corner : frustum_corners) {
            center += glm::vec3(corner);
        }
        center /= static_cast<float>(frustum_corners.size());

        // Create light view matrix
		glm::vec3 light_dir = glm::normalize(light_direction);

		// Choose stable up vector
		glm::vec3 up(0.0f, 1.0f, 0.0f);
		if (glm::abs(glm::dot(light_dir, up)) > 0.99f)
			up = glm::vec3(1.0f, 0.0f, 0.0f);

		// Build light view matrix
		// -Z will point along light_dir
		glm::mat4 light_view = glm::lookAt(
			center - light_dir * 100.0f, // eye
			center,                      // target
			up
		);


        // Transform frustum corners to light space
        float min_x = std::numeric_limits<float>::max();
        float max_x = std::numeric_limits<float>::lowest();
        float min_y = std::numeric_limits<float>::max();
        float max_y = std::numeric_limits<float>::lowest();
        float min_z = std::numeric_limits<float>::max();
        float max_z = std::numeric_limits<float>::lowest();

        for (const auto& corner : frustum_corners) {
            glm::vec4 light_space_corner = light_view * corner;
            min_x = std::min(min_x, light_space_corner.x);
            max_x = std::max(max_x, light_space_corner.x);
            min_y = std::min(min_y, light_space_corner.y);
            max_y = std::max(max_y, light_space_corner.y);
            min_z = std::min(min_z, light_space_corner.z);
            max_z = std::max(max_z, light_space_corner.z);
        }

        // Extend the Z range to include shadow casters behind camera
        float z_mult = 10.0f;
        if (min_z < 0) {
            min_z *= z_mult;
        }
        else {
            min_z /= z_mult;
        }
        if (max_z < 0) {
            max_z /= z_mult;
        }
        else {
            max_z *= z_mult;
        }

        glm::mat4 light_projection = glm::ortho(
            min_x, max_x,
            min_y, max_y,
            min_z, max_z
        );

        return light_projection * light_view;
    }



	void bind_uniform(std::type_index a_type, const std::string& a_location, UniformValue a_value_ptr); // good enough, lowers complexity
	void render_lighting_quad();
	void generate_lighting_quad();
	void generate_lighting_shader();
	void create_pointlight_SSBO();
	void update_pointlight_SSBO(const std::vector<PointLightSSBO>& a_point_lights);
	void create_shadow_framebuffers();

	void create_instance_SSBO();
	void update_insance_SSBO(const std::vector<glm::mat4>& a_model_matrices); bool instance_half;
	void deffered_geometry_pass(const RenderContext& a_render_context, const FrameBuffer* g_buffer);
	void deffered_shadowmap_pass(const RenderContext& a_render_context, const FrameBuffer* shadow_array, size_t a_shadowmap_index);

	

	void set_vec1f(const float value, const char* location) const;
	void set_vec2f(const float value[2], const char* location) const;
	void set_vec3f(const float value[3], const char* location) const;
	void set_vec4f(const float value[4], const char* location) const;
	void set_vec1fv(const std::vector<glm::vec1>& a_value, GLsizei a_count, const char* a_location) const;
	void set_vec2fv(const std::vector<glm::vec2>& a_value, GLsizei a_count, const char* a_location) const;
	void set_vec3fv(const std::vector<glm::vec3>& a_value, GLsizei a_count, const char* a_location) const;
	void set_vec4fv(const std::vector<glm::vec4>& a_value, GLsizei a_count, const char* a_location) const;

	void set_vec1i(const int value, const char* location) const;
	void set_vec2i(const int value[2], const char* location) const;
	void set_vec3i(const int value[3], const char* location) const;
	void set_vec4i(const int value[4], const char* location) const;

	void set_vec1ui(const unsigned int value, const char* location) const;
	void set_vec2ui(const unsigned int value[2], const char* location) const;
	void set_vec3ui(const unsigned int value[3], const char* location) const;
	void set_vec4ui(const unsigned int value[4], const char* location) const;

	void set_mat4fv(glm::mat4 value, GLsizei a_count, const char* location) const;
private:
	GLuint current_gl_program = 0;
	bool not_bound = true;

	GLuint lighting_quad_vao = 0;
	GLuint lighting_quad_vbo = 0;

	GLuint lighting_program = 0;

	GLuint pointlight_ssbo = 0;
	GLuint model_instance_ssbo = 0;

	std::string lighting_shader_path = "./assets/shaders/engine/lighting.shdr";
	unsigned int count = 0;
	glm::vec3 camera_position = glm::vec3(0);
	glm::mat4 proj = glm::mat4(1);
	glm::mat4 view = glm::mat4(1);

	float directional_shadow_resolution = 2048;
	float rect_shadow_resoltuion = 512;
	float spot_shadow_resulution = 512;
	//std::vector<FrameBuffer*> directional_shadow_maps;
	//std::vector<FrameBuffer*> rect_shadow_maps;
	//std::vector<FrameBuffer*> spot_shadow_maps;
	//std::vector<FrameBuffer*> point_shadow_maps;

	FrameBuffer* shadow_maps = nullptr;

	std::shared_ptr<GLFWContext> glfw_context;
};

