#include <engine/resources/data_storage.h>








//void MeshDataGenerationStorage::sort_data()
//{
//	//sort by meshID ... or by shader program... or by shader > mesh_id or something
//	std::sort(storage_data.begin(), storage_data.end(), [](const std::shared_ptr<MeshData>& mesh_a, const std::shared_ptr<MeshData>& mesh_b)
//		{
//			if (mesh_a.get()->mesh_id != mesh_b.get()->mesh_id)
//				return mesh_a.get()->mesh_id < mesh_b.get()->mesh_id;
//		}
//	);
//}
//
//std::optional<std::weak_ptr<MeshData>> MeshDataGenerationStorage::get_data_by_id(const LockKey& a_lock, uint32_t a_id)
//{
//	for (size_t i = 0; i < storage_data.size(); i++)
//	{
//		if (storage_data[i].get()->mesh_id == a_id)
//		{
//			return storage_data[i];
//		}
//	}
//	return std::nullopt;
//}
//
//MeshData MeshDataGenerationStorage::generate_from_string(const std::string& a_string)
//{
//	MeshData mesh_data;
//	mesh_data.mesh_id = get_next_id();
//	return mesh_data;
//}
