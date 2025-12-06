#include "Texture.h"
#include <engine/utilities/macros.h>

Texture::Texture(const char* a_path, const GLenum a_wrap_x, const GLenum a_wrap_y, const GLenum filtering_min, const GLenum filtering_mag, const GLenum mipmap_filtering, bool a_mipmap)
    : wrap_x(a_wrap_x), wrap_y(a_wrap_y), filtering_min(filtering_min), filtering_mag(filtering_mag), generate_mipmap(mipmap_filtering), path(a_path), hashed_path(a_path) {}

Texture::Texture(const char* a_path, const GLenum a_wrap_x, const GLenum a_wrap_y, const GLenum a_filtering_min, const GLenum a_fintering_mag, bool a_mipmap)
    : wrap_x(a_wrap_x), wrap_y(a_wrap_y), filtering_min(filtering_min), filtering_mag(filtering_mag), path(a_path), hashed_path(a_path) {}


Texture::Texture(const char* a_path, const GLenum a_wrap_x, const GLenum a_wrap_y, bool a_mipmap)
    : wrap_x(a_wrap_x), wrap_y(a_wrap_y), path(a_path), generate_mipmap(a_mipmap), hashed_path(a_path) {}

Texture::Texture(const char* a_path, bool a_mipmap) : path(a_path), generate_mipmap(a_mipmap), hashed_path(a_path) {}


bool Texture::load_from_path(const std::string a_path, bool a_new_name)
{
    if (ram_loaded)
    {
        marked_for_load = false;
        failed_load = false;
        return false;
    }
    if (path == "")
    {
        PRINTLN("No path assigned");
        ram_loaded = false;
        marked_for_load = false;
        failed_load = true;
        return false;
    }
    if (a_new_name)
    {
        path = a_path;
        hashed_path = hashed_string_64(a_path.c_str());
    }
    // load and generate the texture
    int temp_width, temp_height, temp_num_comps;
    stbi_uc* raw_ptr = stbi_load(a_path.c_str(), &temp_width, &temp_height, &temp_num_comps, 0);
    if (raw_ptr == nullptr)
    {
        ram_loaded = false;
        marked_for_load = false;
        failed_load = true;
        return false;
    }
    texture_data.reset(raw_ptr);
    if (temp_num_comps == 1) color_format = GL_RED;
    else if (temp_num_comps == 2) color_format = GL_RG;
    else if (temp_num_comps == 3) color_format = GL_RGB;
    else if (temp_num_comps == 4) color_format = GL_RGBA;
    else
    {
        std::println("Failed to load texture");
        std::println("loaded texture at {}", texture_id);
        return false;
    }
    width = temp_width;
    height = temp_height;
    marked_for_load = false;
    failed_load = false;
    ram_loaded = true;
    return true;
}

bool Texture::load_to_gpu(bool a_delete_ram_loaded)
{
    if (gpu_loaded) return true;
    if (!texture_data)
    {
        PRINTLN("texture data not ram loaded");
        gpu_loaded = false;
        return false;
    }

    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_x);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_y);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering_min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering_mag);
    glTexImage2D(GL_TEXTURE_2D, 0, color_format, width, height, 0, color_format, GL_UNSIGNED_BYTE, texture_data.get());
    if (generate_mipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmap_filtering);
    }
    PRINTLN("loaded texture at {}", texture_id);
    if (a_delete_ram_loaded)
    {
        texture_data.reset();
        ram_loaded = false;
        marked_for_load = false;
        failed_load = false;
    }
    gpu_loaded = true;
    return true;
}

bool Texture::unload_texture()
{
    if (!ram_loaded) return true;
    texture_data.reset();
    ram_loaded = false;
}

bool Texture::delete_gpu_texture()
{
    if (!gpu_loaded) return true;
    else if (texture_id < 1) return true;
    glDeleteTextures(1, &texture_id);
    texture_id = 0;
    gpu_loaded = false;
    return true;
}

void Texture::activate()
{
    if (texture_id <= 0)
    {
        std::println("No texture loaded");
        return;
    }
    if (index < 0)
    {
        std::println("Texture has no index");
        return;
    }
    else if (index > 15)
    {
        std::println("Texture has too high index, {}", index);
        return;
    }
    glActiveTexture((GLenum)(GL_TEXTURE0 + index));
    glBindTexture(GL_TEXTURE_2D, texture_id);
}

