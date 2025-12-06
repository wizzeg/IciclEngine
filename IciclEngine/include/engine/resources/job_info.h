#pragma once
#include <engine/utilities/hashed_string_64.h>

enum EMeshDataRequest : uint8_t
{
	LoadFromFile,
	ReloadFromFile,
	GenerateVAORequest,
	VAODataLoaded,
	DeleteVAORequest,
	UnloadMeshData
};

struct MeshDataJob // will have a texture data job too....
{
	hashed_string_64 path_hashed;
	EMeshDataRequest request_type;
};