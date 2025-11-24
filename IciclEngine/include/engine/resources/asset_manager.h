#pragma once
#include <cstdint>
#include <string>
#include <engine/renderer/render_info.h>
#include <engine/game/components.h>
#include "obj_parser.h"
#include <mutex>

struct MutexStorageArg
{
	const std::string name;
};

struct MutexLock
{
	const std::string name;
	bool locked;
	uint8_t key;
	std::mutex mutex;
};

struct SOmething
{
	std::string path;
	uint32_t mesh_id;
	MeshData mesh;
};

struct AssetManager
{
	AssetManager() {}
	AssetManager(std::vector<MutexStorageArg>& a_mutex_storage_args);
	~AssetManager() {}

	RenderableComponent load_Asset3D(const MeshPath& a_asset) { return RenderableComponent{ 0,0 }; } // hmm, should not have this, I think it makes more sense to have two different ones
	MeshHandle load_mesh(const MeshPath& a_asset) { return MeshHandle{0}; } // remember string& to avoid copy I think, need it elsewhere too
	MeshHandle load_obj_mesh(ObjMesh& a_mesh) { return MeshHandle{ 0 }; }
	MaterialHandle load_material(const MeshPath& a_asset) { return MaterialHandle{ 0 }; }
	std::uint32_t get_vao(MeshHandle a_handle) { return 0; }
};

