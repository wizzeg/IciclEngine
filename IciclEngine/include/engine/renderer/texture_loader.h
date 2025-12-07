#pragma once
#include <engine/renderer/render_info.h>
struct TextureLoader
{
	TextureGenInfo generate_texture(TextureData& a_texture_data);
	void delete_texture(TextureGenInfo& a_texture_info);
};

