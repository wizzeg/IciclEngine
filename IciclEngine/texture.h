#pragma once
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <algorithm>
#include <string>
#include <memory>
#include <engine/utilities/hashed_string_64.h>
#include <stb_image/stb_image.h>

struct Texture
{
	Texture(const char* a_path, bool a_mipmap = true);
	Texture(const char* a_path, const GLenum a_wrap_x, const GLenum a_wrap_y, bool a_mipmap = true);
	Texture(const char* a_path, const GLenum a_wrap_x, const GLenum a_wrap_y, const GLenum a_filtering_min, const GLenum a_fintering_mag, bool a_mipmap = true);
	Texture(const char* a_path, const GLenum a_wrap_x, const GLenum a_wrap_y, const GLenum a_filtering_min, const GLenum a_fintering_mag, const GLenum a_mipmap_filtering, bool a_mipmap = true);

	void set_index(unsigned int a_index) { index = a_index; } // texture slot

	~Texture() {};

	bool load_from_path(const std::string a_path, bool a_new_name);
	bool load_to_gpu(bool a_delete_ram_loaded = false);
	bool unload_texture();
	bool delete_gpu_texture();
	void set_texture_id(GLuint a_texture_id) { texture_id = a_texture_id; }
	GLuint get_texture_id() { return texture_id; }
	void activate();
private:
	
	GLenum wrap_x = GL_REPEAT;
	GLenum wrap_y = GL_REPEAT;
	GLenum filtering_min = GL_NEAREST;
	GLenum filtering_mag = GL_LINEAR;
	bool generate_mipmap = true;
	GLenum mipmap_filtering = GL_LINEAR_MIPMAP_LINEAR;
	float border_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	GLenum color_format = GL_RGB;

	std::shared_ptr<stbi_uc> texture_data;

	bool failed_load = false;
	bool marked_for_load = false;
	bool ram_loaded = false;
	bool gpu_loaded = false;
	GLuint texture_id = 0;
	GLsizei width = 0;
	GLsizei height = 0;

	std::string path = " ";
	hashed_string_64 hashed_path;
	unsigned int index = 0;
};

