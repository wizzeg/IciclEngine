#include <engine/game/camera.h>
//#include <glm/ext/matrix_transform.hpp>
//#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <engine/utilities/macros.h>
#include <glm/mat4x4.hpp>
// #include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

void Camera::move(glm::vec2 delta_mouse)
{
	if (!movable) return;

	float yaw_angle = glm::radians(delta_mouse.x * camera_sensitivity * -1.0f);
	float pitch_angle = glm::radians(delta_mouse.y * camera_sensitivity);

	glm::quat yaw_quat = glm::angleAxis(yaw_angle, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec3 right = glm::normalize(glm::mat3_cast(rotation_quaternion) * glm::vec3(1, 0, 0));
	glm::quat pitch_quat = glm::angleAxis(pitch_angle, right);
	glm::quat rot_quat = yaw_quat * pitch_quat;
	rotation_quaternion = glm::normalize(rot_quat * rotation_quaternion);
	proj_view_unchanged = false;
}

void Camera::move(glm::vec3 input)
{
	if (!movable) return;
	glm::vec3 rotated_input = rotation_quaternion * input;
	camera_pos += rotated_input;
	//camera_target += input;
	proj_view_unchanged = false;
}

void Camera::update_proj_view_matrix()
{
	glm::mat4 rotation_matrix = glm::mat4_cast(rotation_quaternion);
	glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), -camera_pos);
	view = glm::transpose(rotation_matrix) * translation_matrix;
	proj = glm::perspective(glm::radians(field_of_view), ((float)camera_aspect.x / (float)camera_aspect.y), 0.1f, 300.0f);
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
