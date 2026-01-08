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

enum EAssetJob : uint8_t
{
	MeshAssetJob,
	TextureAssetJob,
	ShaderAssetJob,
	GPUMeshAssetJob,
	GPUTextureJob,
	GPUShaderJob,
	MaterialAssetJob
};

struct AssetJobStruct // this is really the only one I need ... need to record time .. doesn't work, e.g. vao load inf requires special data
{
	hashed_string_64 path_hashed;
	uint64_t job_time;
	ERequestType request_type;
	EAssetJob assetjob_type;
	
};

struct MeshDataJob // will have a texture data job too....
{
	hashed_string_64 path_hashed;
	ERequestType request_type;
	uint64_t job_time;
};

struct MaterialDataJob
{
	hashed_string_64 path_hashed;
	ERequestType request_type;
	uint64_t job_time;
};

struct TextureDataJob
{
	hashed_string_64 path_hashed;
	ERequestType request_type;
	uint64_t job_time;
};

struct ShaderDataJob
{
	hashed_string_64 path_hashed;
	ERequestType request_type;
	uint64_t job_time;
};