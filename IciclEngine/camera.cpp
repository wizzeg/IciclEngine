#include "camera.h"
//#include <glm/ext/matrix_transform.hpp>
//#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <engine/utilities/macros.h>
#include <glm/mat4x4.hpp>

void Camera::move(glm::vec2 delta_mouse)
{
	if (!movable) return;
	rot_x += delta_mouse.y;
	rot_z += delta_mouse.x;
	proj_view_unchanged = false;
}

void Camera::move(glm::vec3 input)
{
	if (!movable) return;
	camera_pos += input;
	camera_target += input;
	proj_view_unchanged = false;
}

void Camera::update_proj_view_matrix()
{
	// when up changes this must also adjust for that.
	glm::vec3 direction = glm::normalize(camera_pos - camera_target);
	glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0.f, 1.f, 0.f)));
	glm::vec3 up = glm::cross(right, direction);
	view = glm::lookAt(camera_pos, camera_target, up);
	proj = glm::perspective(glm::radians(70.0f), ((float)camera_aspect.y / (float)camera_aspect.x), 0.1f, 1000.0f);
	proj_view_unchanged = true;
}

glm::mat4 Camera::get_view_matrix()
{
	if (proj_view_unchanged)
	{
		return view;
	}
	update_proj_view_matrix();
	return view;
}

glm::mat4 Camera::get_proj_matrix()
{
	if (proj_view_unchanged)
	{
		return proj;
	}
	update_proj_view_matrix();
	return proj;
}

void Camera::set_camera_movable(bool a_movable)
{
	movable = a_movable;
}
