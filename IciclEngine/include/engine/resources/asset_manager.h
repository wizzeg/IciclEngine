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
	
	// every thing is gonna ask for a load, that means multiple messages with the same string
	//


	//message idea, mesh does not exist = 0, mesh is loading is uint32_t.max
	//multiple threads for mesh loading? I may time this for when something like ECB is adding/removing components/entities (single threaded)... no loading takes too long.
	//just one thread for mesh loading... but then I won't use mutex. Perhaps fine for now. I don't know when I'd use mutexes...


	RenderableComponent load_Asset3D(const Asset3D& a_asset) { return RenderableComponent{ 0,0 }; } // hmm, should not have this, I think it makes more sense to have two different ones
	MeshHandle load_mesh(const Asset3D& a_asset) { return MeshHandle{0}; } // remember string& to avoid copy I think, need it elsewhere too
	MeshHandle load_obj_mesh(ObjMesh& a_mesh) { return MeshHandle{ 0 }; }
	MaterialHandle load_material(const Asset3D& a_asset) { return MaterialHandle{ 0 }; }
	std::uint32_t get_vao(MeshHandle a_handle) { return 0; }
};

