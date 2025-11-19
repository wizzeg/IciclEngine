#pragma once
#include <cstdint>
#include <string>
#include <engine/renderer/render_info.h>
#include <engine/game/components.h>
#include "obj_parser.h"

struct AssetManager
{
	AssetManager() {}
	~AssetManager() {}

	RenderableComponent load_Asset3D(const Asset3D& a_asset) { return RenderableComponent{ 0,0 }; } // hmm, should not have this, I think it makes more sense to have two different ones
	MeshHandle load_mesh(const Asset3D& a_asset) { return MeshHandle{0}; } // remember string& to avoid copy I think, need it elsewhere too
	MeshHandle load_obj_mesh(ObjMesh& a_mesh) { return MeshHandle{ 0 }; }
	MaterialHandle load_material(const Asset3D& a_asset) { return MaterialHandle{ 0 }; }
	std::uint32_t get_vao(MeshHandle a_handle) { return 0; }
};

