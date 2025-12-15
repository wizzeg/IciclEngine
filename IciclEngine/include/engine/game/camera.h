#pragma once
#include <engine/renderer/frame_buffer.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <engine/utilities/hashed_string_64.h>
#include <glm/gtc/quaternion.hpp>
struct Camera
{
	Camera(const std::string a_name, int a_width, int a_height) :name(a_name), camera_aspect((float)a_width, (float)a_height), hashed_name(a_name.c_str()) //:frame_buffer(a_name, a_width, a_height), name(a_name)
	{
	}
	// this should have the frame buffer
	// should it have a renderer too? -> then it needs scene... but entt does the render calls -> no should not have scene.
	// should entities/scene_objects control camera? Not editor camera at least
	void move(glm::vec2 delta_mouse);
	void move(glm::vec3 input);

	void update_proj_view_matrix();
	glm::mat4 get_view_matrix();
	glm::mat4 get_proj_matrix();
	void set_camera_movable(bool a_movable);
	float get_camera_speed() const { return camera_speed; }
	glm::vec3 get_camera_positoin() { return camera_pos; }
private:
	// final color output perhaps? Renderer would have all those other frame buffers, and only output final frame buffer to the camera..
	// but renderer also has to generate all those other frame buffers from perspective of camera
	std::string name;
	hashed_string_64 hashed_name;
	//FrameBuffer frame_buffer;
	glm::vec3 camera_pos = glm::vec3(0.0f, 0.0f, 10.0f);
	glm::vec3 camera_target = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::quat rotation_quaternion = glm::quat(1.f, 0, 0, 0);

	glm::ivec2 camera_aspect;

	glm::mat4 view = glm::mat4(1);
	glm::mat4 proj = glm::mat4(1);

	float rot_x = 0;
	float rot_z = 0;
	float field_of_view = 70.0f; 
	float camera_sensitivity = 0.1f;
	float camera_speed = 2.f;

	bool movable = false;
	bool proj_view_unchanged = false;

	unsigned int frame_buffer = 0;
	unsigned int color_texture = 0;
};

