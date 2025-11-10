#include "Texture.h"
#include <print>
#include "stb_image/stb_image.h"

bool Texture::Load()
{
    if (Path == nullptr)
    {
        std::println("No path assigned");
        return false;
    }

    glGenTextures(1, &TextureID);
    glBindTexture(GL_TEXTURE_2D, TextureID);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, WrapX);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, WrapY);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, FilteringMin);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, FilteringMag);
    // load and generate the texture
    int width, height, nrChannels;
    unsigned char* data = stbi_load(Path, &width, &height, &nrChannels, 0);
    bool result = (data);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, ColorFormat, width, height, 0, ColorFormat, GL_UNSIGNED_BYTE, data);
        if (generateMipMap)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, MipMapFiltering);
        }
        
    }
    else
    {
        std::println("Failed to load texture");
    }
    stbi_image_free(data);
    loaded = (result);
    return result;
}

void Texture::Activate()
{
    if (TextureID <= 0)
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
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_2D, TextureID);
}