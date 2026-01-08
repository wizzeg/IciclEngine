#include <engine/renderer/texture_loader.h>
#include <engine/utilities/macros.h>

TextureGenInfo TextureLoader::generate_texture(TextureData& a_texture_data)
{
    TextureGenInfo texture_info(a_texture_data.contents->hashed_path, 0, ELoadStatus::FailedLoadBadPath);
    if (a_texture_data.texture_ram_status != ELoadStatus::Loaded)
    {
        a_texture_data.texture_gen_status = ELoadStatus::FailedLoadBadPath;
        return texture_info;
    }
    glGenTextures(1, &a_texture_data.texture_id);
    glBindTexture(GL_TEXTURE_2D, a_texture_data.texture_id);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, a_texture_data.contents->wrap_x);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, a_texture_data.contents->wrap_y);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, a_texture_data.contents->filtering_min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, a_texture_data.contents->filtering_mag);
    glTexImage2D(GL_TEXTURE_2D, 0, a_texture_data.contents->color_format, a_texture_data.contents->width,
        a_texture_data.contents->height, 0, a_texture_data.contents->color_format,
        GL_UNSIGNED_BYTE, a_texture_data.contents->texture_data.get());
    if (a_texture_data.contents->generate_mipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, a_texture_data.contents->mipmap_filtering);
    }
    a_texture_data.texture_gen_status = ELoadStatus::Loaded;
    texture_info.texture_gen_status = ELoadStatus::Loaded;
    texture_info.texture_id = a_texture_data.texture_id;
    return texture_info;
}

void TextureLoader::delete_texture(TextureGenInfo& a_texture_info)
{
    a_texture_info.texture_gen_status = ELoadStatus::NotLoaded;
    if (a_texture_info.texture_id == 0) return;
    glDeleteTextures(1, &a_texture_info.texture_id);
    a_texture_info.texture_id = 0;
}
