#include <engine/resources/data_storage.h>

MeshJobProcessor::MeshJobProcessor(ModelGenStorage& a_gen_storage) : meshjob_gen_storage(a_gen_storage) {}
void MeshJobProcessor::process_job(MeshDataJob& a_job)
{
	if (a_job.request_type == EMeshDataRequest::LoadFromFile || a_job.request_type == EMeshDataRequest::ReloadFromFile)
	{
		bool found_data = false;
		bool start_load = false;
		{
			std::unique_lock <std::mutex> data_lock(*meshjob_gen_storage.mesh_mutex);
			auto& datas = *meshjob_gen_storage.mesh_datas;

			// look if data already exists
			for (size_t i = 0; i < datas.size(); i++)
			{
				if (a_job.path_hashed == datas[i].path_hashed)
				{
					found_data = true;
					auto& data = datas[i];
					switch (data.ram_load_status) // what to do depending on what the ram load status is
					{
					case EMeshDataRAMLoadStatus::FailedBadModel: // fall through
					case EMeshDataRAMLoadStatus::FailedOpen:
						// deal with error
						break;
					case EMeshDataRAMLoadStatus::FailedNoSpace: // fall through
					case EMeshDataRAMLoadStatus::NotRAMLoaded:
						start_load = true;
						data.ram_load_status = EMeshDataRAMLoadStatus::StartedLoad;
						break;
					default:
						break;
					}
				}
			}
			if (!found_data) // the mesh data doesn't exist, so we add and claim it for loading it
			{
				meshjob_gen_storage.mesh_datas->emplace_back();
				auto& data = meshjob_gen_storage.mesh_datas->back();
				data.path_hashed = a_job.path_hashed;
				data.path = data.path_hashed.string;
				data.ram_load_status = EMeshDataRAMLoadStatus::StartedLoad;
				start_load = true;
				sort_data(data_lock);
			}
		}
		if (start_load) // if we should load we load the mesh data from file
		{
			MeshData data = obj_parser.load_mesh_from_filepath(a_job.path_hashed.string);
			{
				std::unique_lock<std::mutex> data_lock(*meshjob_gen_storage.mesh_mutex);
				if (data.ram_load_status == EMeshDataRAMLoadStatus::LoadedRAM)
				{
					data.vao_load_status = EMeshDataVAOLoadStatus::RequstedVAOLoad;
					meshjob_gen_storage.vao_requester->add_message(VAOLoadRequest{ data }); // copying data for the VAOLoadRequest
				}
				auto& datas = *meshjob_gen_storage.mesh_datas;
				bool found_data = false;       // Make sure that we add it, this should always exist, but safety check
				for (size_t i = 0; i < datas.size(); i++)
				{
					if (a_job.path_hashed == datas[i].path_hashed)
					{
						datas[i] = data;
						found_data = true;
						break;
					}
				}
				if (!found_data) // This should not happen, but we have to add it
				{
					datas.push_back(data);
					sort_data(data_lock);
				}
			}
		}
	}
	
}

void MeshJobProcessor::sort_data(std::unique_lock<std::mutex>& a_lock)
{
	if (a_lock.owns_lock())
	{
		std::sort(meshjob_gen_storage.mesh_datas->begin(), meshjob_gen_storage.mesh_datas->end(), [](const MeshData& mesh_a, const MeshData& mesh_b)
			{
				return mesh_a.path_hashed < mesh_b.path_hashed;
			});
	}
}

VAOInfoProcessor::VAOInfoProcessor(std::shared_ptr<std::mutex> a_data_mutex, std::shared_ptr<std::vector<MeshData>> a_datas)
	: vao_processor_data_mutex(a_data_mutex), vao_processor_datas(a_datas) {}

void VAOInfoProcessor::process_job(VAOLoadInfo& a_job)
{
	if (a_job.vao_loaded)
	{
		std::unique_lock<std::mutex> data_lock(*vao_processor_data_mutex);
		auto& datas = *vao_processor_datas;
		bool found_data = false;
		for (size_t i = 0; i < datas.size(); i++)
		{
			if (a_job.hashed_path == datas[i].path_hashed)
			{
				found_data = true;
				datas[i].vao_load_status = EMeshDataVAOLoadStatus::VAOLoaded; /// I need the actual data too...
			}
		}
		if (!found_data)
		{
			MeshData new_mesh;
			new_mesh.vao_load_status = EMeshDataVAOLoadStatus::VAOLoaded;
			vao_processor_datas->push_back(new_mesh);
			sort_data(data_lock);
		}
	}
	else
	{
		std::unique_lock<std::mutex> data_lock(*vao_processor_data_mutex);
		auto& datas = *vao_processor_datas;
		bool found_data = false;
		for (size_t i = 0; i < datas.size(); i++)
		{
			if (a_job.hashed_path == datas[i].path_hashed)
			{
				found_data = true;
				datas[i].vao_load_status = EMeshDataVAOLoadStatus::NotVAOLoaded; /// I need the actual data too...
			}
		}
		if (!found_data)
		{
			MeshData new_mesh;
			new_mesh.vao_load_status = EMeshDataVAOLoadStatus::NotVAOLoaded;
			vao_processor_datas->push_back(new_mesh);
			sort_data(data_lock);
		}
	}
}

void VAOInfoProcessor::sort_data(std::unique_lock<std::mutex>& a_lock)
{
	if (a_lock.owns_lock())
	{
		std::sort(vao_processor_datas->begin(), vao_processor_datas->end(), [](const MeshData& mesh_a, const MeshData& mesh_b)
			{
				return mesh_a.path_hashed < mesh_b.path_hashed;
			});
	}
}

GenStorageThreadPool::GenStorageThreadPool(size_t num_threads) 
	: job_messages(std::make_shared<MessageQueue<LoadJob>>()), cv_new_job()
{
	for (size_t i = 0; i < num_threads; i++)
		threads.emplace_back([this, i] { worker_loop(i); });
}

GenStorageThreadPool::~GenStorageThreadPool()
{
	exit = true;
	cv_new_job.notify_all();
	for (auto& thread : threads)
		if (thread.joinable())
			thread.join();
}

void GenStorageThreadPool::add_job(LoadJob& a_job)
{
	job_messages->add_message(a_job);
	cv_new_job.notify_one();
}

void GenStorageThreadPool::add_jobs(std::vector<LoadJob>& a_jobs)
{
	job_messages->add_messages(a_jobs);
	for (size_t i = 0; i < a_jobs.size(); i++)
		cv_new_job.notify_one();
}

void GenStorageThreadPool::clear_jobs()
{
	job_messages->clear_messages();
}

ModelGenStorage::ModelGenStorage(size_t num_threads)
	: GenStorageThreadPool(num_threads),  mesh_mutex(std::make_shared<std::mutex>()), mesh_datas(std::make_shared<std::vector<MeshData>>()), vao_requester(std::make_shared<MessageQueue<VAOLoadRequest>>()),
	mesh_job_processor(*this), vaoinfo_job_processor(mesh_mutex, mesh_datas), render_request_returner(mesh_datas, mesh_mutex), vaoload_returner(vao_requester) {}

const std::type_info& ModelGenStorage::get_cast_type()
{
	return typeid(ModelGenStorage);
}

//std::optional<VAOLoadRequest> ModelGenStorage::get_vao_request() { return vao_requester->get_message(); }
//void ModelGenStorage::fulfilled_vao_request(LoadJob& a_job) { add_job(a_job); }
void ModelGenStorage::worker_loop(size_t a_thread_id)
{
	while (!exit)
	{
		if (auto a_job = job_messages->get_message())
		{
			LoadJob& job = a_job.value();

			if (auto mesh_job = std::get_if<MeshDataJob>(&job))
			{
				mesh_job_processor.process_job(*mesh_job);
			}
			else if (auto vao_job = std::get_if<VAOLoadInfo>(&job))
			{
				vaoinfo_job_processor.process_job(*vao_job);
			}
			/// Now just add everything for textures to be processed and loaded etc...
		}
		{
			std::unique_lock<std::mutex> thread_lock(thread_mutex);
			cv_new_job.wait(thread_lock, [this] { return worker_wait_condition(); });
		}
	}
}

bool ModelGenStorage::worker_wait_condition()
{
	bool result = false;
	result |= !job_messages->is_empty();
	result |= exit;
	return result;
}

std::optional<RenderRequest> RenderRequestReturner::return_request(const hashed_string_64& a_request)
{
	{
		std::lock_guard<std::mutex> guard(*render_request_returner_mesh_mutex);
		auto& datas = *render_request_returner_mesh_datas;
		for (size_t i = 0; i < datas.size(); i++)
		{
			if (a_request == datas[i].path_hashed)
			{
				return RenderRequest(a_request, datas[i].VAO, (GLuint)datas[i].indices.size(), glm::mat4(1), 0, 0);
			}
		}
	}
	return std::nullopt;
}

std::optional<std::vector<RenderRequest>> RenderRequestReturner::return_requests(std::vector<hashed_string_64>& a_request, bool sorted) // does not require pre-sorting
{
	std::vector<RenderRequest> render_requests;
	if (!sorted)
		std::sort(a_request.begin(), a_request.end());

	render_requests.reserve(a_request.size());
	{
		auto& datas = *render_request_returner_mesh_datas;
		size_t start_index = 0;
		size_t mesh_index = 0;
		bool found = false;
		std::lock_guard<std::mutex> guard(*render_request_returner_mesh_mutex);
		for (auto& request : a_request)
		{
			start_index = mesh_index;
			found = false;
			for (; mesh_index < datas.size(); mesh_index++)
			{
				start_index = mesh_index;
				if (request == datas[mesh_index].path_hashed)
				{
					render_requests.emplace_back(request, datas[mesh_index].VAO, (GLuint)datas[mesh_index].indices.size(), glm::mat4(1), 0, 0);
					found = true;
					break;
				}
			}
			if (!found) mesh_index = start_index;
		}


	}
	if (render_requests.empty())
	{
		return std::nullopt;
	}
	return render_requests;
	
}
