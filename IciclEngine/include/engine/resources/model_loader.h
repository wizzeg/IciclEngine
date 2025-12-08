#pragma once

#include <engine/renderer/render_info.h>



struct ModelLoader
{
	MeshData load_obj_mesh_from_file(const std::string a_path);
	void load_texture_from_file(TextureData& a_texture_data);
	TextureData load_texture_from_file(const std::string a_path, bool a_mipmap = true);
	TextureData load_texture_from_file(const std::string a_path, const GLenum a_wrap_x, const GLenum a_wrap_y, bool a_mipmap = true);
	TextureData load_texture_from_file(const std::string a_path, const GLenum a_wrap_x, const GLenum a_wrap_y, const GLenum a_filtering_min, const GLenum a_fintering_mag, bool a_mipmap = true);
	TextureData load_texture_from_file(const std::string a_path, const GLenum a_wrap_x, const GLenum a_wrap_y, const GLenum a_filtering_min, const GLenum a_fintering_mag, const GLenum a_mipmap_filtering, bool a_mipmap = true);
};

