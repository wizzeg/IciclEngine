#pragma once
#include <engine/utilities/hashed_string_64.h>

enum EMeshDataRequest : uint8_t
{
	LoadMeshFromFile,
	ReloadMeshFromFile,
	GenerateVAORequest,
	VAODataLoaded,
	DeleteVAORequest,
	UnloadMeshData
};
enum ETextureDataRequest : uint8_t
{
	LoadTextureFromFile,
	ReloadTextureFromFile,
	GenerateTextureRequest,
	UpdateTextureBindID
};

struct MeshDataJob // will have a texture data job too....
{
	hashed_string_64 path_hashed;
	EMeshDataRequest request_type;
};

struct TextureDataJob
{
	hashed_string_64 path_hashed;
	ETextureDataRequest request_type;
};