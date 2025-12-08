#pragma once
#include <engine/utilities/hashed_string_64.h>
enum EDirection : uint8_t
{
	None,
	Up,
	Down
};

enum ERequestType :uint8_t
{
	LoadFromFile,
	ReloadFromFile,
	DeleteFromRam,
	DeleteFromGPU,
	LoadToGPU,
	UpdateGPUInfo,
	UpdateRAMInfo,
	BindToGPU
};

struct MeshDataJob // will have a texture data job too....
{
	hashed_string_64 path_hashed;
	ERequestType request_type;
};

struct TextureDataJob
{
	hashed_string_64 path_hashed;
	ERequestType request_type;
};