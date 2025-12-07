#include <engine/resources/data_storage.h>
#include <engine/utilities/macros.h>

MeshJobProcessor::MeshJobProcessor(ModelGenStorage& a_gen_storage) : meshjob_gen_storage(a_gen_storage) {}
void MeshJobProcessor::process_job(MeshDataJob& a_job)
{
	if (a_job.request_type == EMeshDataRequest::LoadMeshFromFile || a_job.request_type == EMeshDataRequest::ReloadMeshFromFile)
	{
		bool found_data = false;
		bool start_load = false;
		{
			std::unique_lock <std::mutex> data_lock(meshjob_gen_storage.mesh_mutex);
			auto& datas = meshjob_gen_storage.mesh_datas;

			// look if data already exists
			for (size_t i = 0; i < datas.size(); i++)
			{
				if (a_job.path_hashed == datas[i].path_hashed)
				{
					found_data = true;
					auto& data = datas[i];
					switch (data.ram_load_status) // what to do depending on what the ram load status is
					{
					case ELoadStatus::FailedLoadBadModel: // fall through
					case ELoadStatus::FailedLoadOpen:
						// deal with error
						break;
					case ELoadStatus::FailedLoadNoSpace: // fall through
					case ELoadStatus::NotLoaded:
						start_load = true;
						data.ram_load_status = ELoadStatus::StartedLoad;
						break;
					default:
						break;
					}
				}
			}
			if (!found_data) // the mesh data doesn't exist, so we add and claim it for loading it
			{
				meshjob_gen_storage.mesh_datas.emplace_back();
				auto& data = meshjob_gen_storage.mesh_datas.back();
				data.path_hashed = a_job.path_hashed;
				data.path = data.path_hashed.string;
				data.ram_load_status = ELoadStatus::StartedLoad;
				start_load = true;
				sort_data(data_lock);
			}
		}
		if (start_load) // if we should load we load the mesh data from file
		{
			MeshData data = obj_parser.load_mesh_from_filepath(a_job.path_hashed.string);
			{
				if (data.ram_load_status == ELoadStatus::Loaded)
				{
					data.vao_load_status = ELoadStatus::RequestedLoad;
					meshjob_gen_storage.vaoload_requests.add_message(VAOLoadRequest{ data }); // copying data for the VAOLoadRequest
				}
				std::unique_lock<std::mutex> data_lock(meshjob_gen_storage.mesh_mutex);
				auto& datas = meshjob_gen_storage.mesh_datas;
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
		std::sort(meshjob_gen_storage.mesh_datas.begin(), meshjob_gen_storage.mesh_datas.end(), [](const MeshData& mesh_a, const MeshData& mesh_b)
			{
				return mesh_a.path_hashed < mesh_b.path_hashed;
			});
	}
}

VAOInfoProcessor::VAOInfoProcessor(ModelGenStorage& a_gen_storage) : vaoinfo_gen_storage(a_gen_storage){}

void VAOInfoProcessor::process_job(VAOLoadInfo& a_job)
{
	if (a_job.vao_loaded)
	{
		std::unique_lock<std::mutex> data_lock(vaoinfo_gen_storage.mesh_mutex);
		auto& datas = vaoinfo_gen_storage.mesh_datas;
		bool found_data = false;
		for (size_t i = 0; i < datas.size(); i++)
		{
			if (a_job.hashed_path == datas[i].path_hashed)
			{
				auto& data = datas[i];
				found_data = true;
				data.vao_load_status = ELoadStatus::Loaded; /// I need the actual data too...
				data.VAO = a_job.vao;
				data.EBO = a_job.ebo;
				data.VBOs = a_job.VBOs;
			}
		}
		if (!found_data)
		{
			MeshData new_mesh;
			new_mesh.vao_load_status = ELoadStatus::Loaded;
			new_mesh.VAO = a_job.vao;
			new_mesh.EBO = a_job.ebo;
			new_mesh.VBOs = a_job.VBOs;
			datas.push_back(new_mesh);
			sort_data(data_lock);
		}
	}
	else
	{
		std::unique_lock<std::mutex> data_lock(vaoinfo_gen_storage.mesh_mutex);
		auto& datas = vaoinfo_gen_storage.mesh_datas;
		bool found_data = false;
		for (size_t i = 0; i < datas.size(); i++)
		{
			if (a_job.hashed_path == datas[i].path_hashed)
			{
				found_data = true;
				datas[i].vao_load_status = ELoadStatus::NotLoaded; /// I need the actual data too...
			}
		}
		if (!found_data)
		{
			MeshData new_mesh;
			new_mesh.vao_load_status = ELoadStatus::NotLoaded;
			datas.push_back(new_mesh);
			sort_data(data_lock);
		}
	}
}

TextureDataProcessor::TextureDataProcessor(ModelGenStorage& a_gen_storage) : texturedata_gen_storage(a_gen_storage)
{
}

void TextureDataProcessor::process_job(TextureDataJob& a_job)
{
}

void VAOInfoProcessor::sort_data(std::unique_lock<std::mutex>& a_lock)
{
	if (a_lock.owns_lock())
	{
		std::sort(vaoinfo_gen_storage.mesh_datas.begin(), vaoinfo_gen_storage.mesh_datas.end(), [](const MeshData& mesh_a, const MeshData& mesh_b)
			{
				return mesh_a.path_hashed < mesh_b.path_hashed;
			});
	}
}

GenStorageThreadPool::GenStorageThreadPool(size_t a_num_threads) : num_threads(a_num_threads), job_messages(std::make_shared<MessageQueue<LoadJob>>()) // suspect something might be wrong with the job message?
{
}

GenStorageThreadPool::~GenStorageThreadPool()
{
	{
		std::lock_guard<std::mutex> thread_guard(thread_mutex);
		exit = true;
		start = true;
	}
	cv_new_job.notify_all();
	cv_start_threads.notify_all();
	for (auto& thread : threads)
		if (thread.joinable())
			thread.join();
}

void GenStorageThreadPool::start_threads()
{
	for (size_t i = 0; i < num_threads; i++)
		threads.emplace_back([this, i] { worker_loop(i); }); // complained here on random crashes....
	{
		std::lock_guard<std::mutex> start_guard(thread_mutex);
		start = true;
	}
	cv_start_threads.notify_all();
}

void GenStorageThreadPool::add_job(LoadJob& a_job)
{
	job_messages->add_message(a_job);
	cv_new_job.notify_one();
}

void GenStorageThreadPool::add_jobs(std::vector<LoadJob>& a_jobs)
{
	job_messages->add_messages(a_jobs);
	if (a_jobs.size() > 1) cv_new_job.notify_all();
	else cv_new_job.notify_one();
}

void GenStorageThreadPool::clear_jobs()
{
	job_messages->clear_messages();
}

ModelGenStorage::ModelGenStorage(size_t num_threads)
	: GenStorageThreadPool(num_threads), mesh_job_processor(*this), vaoinfo_job_processor(*this), render_request_returner(*this), vaoload_returner(*this)
{
	start_threads();
	// Possible cause of crash. When GenSotrageThreadPool created threads in constructor, then "this" might point to an incomplete object
	// then when it tries to run worker_loop, that pointer has not yet been created, the v table might still point to = 0, instead of worker_loop
}

const std::type_info& ModelGenStorage::get_cast_type() // not actually using this
{
	return typeid(ModelGenStorage);
}

void ModelGenStorage::worker_loop(size_t a_thread_id)
{
	size_t thread_id = a_thread_id;
	{
		PRINTLN("loader thread: {} - waiting to start", a_thread_id);
		std::unique_lock<std::mutex> thread_lock(thread_mutex);
		cv_start_threads.wait(thread_lock, [this] { return start || exit; });
	}
	PRINTLN("loader thread: {} - confirmed start", a_thread_id);
	while (!exit)
	{
		{
			std::unique_lock<std::mutex> thread_lock(thread_mutex);
			cv_new_job.wait(thread_lock, [this, thread_id]
				{
					bool result = worker_wait_condition();
					if (!result)
					{
						PRINTLN("loader thread: {} - going to sleep", thread_id);
					}
					return  result;
				});
		}
		PRINTLN("loader thread: {} - woke up/getting job", a_thread_id);
		if (auto a_job = job_messages->get_message())
		{
			PRINTLN("loader thread: {} - got job", a_thread_id);
			LoadJob& job = a_job.value();
			if (auto mesh_job = std::get_if<MeshDataJob>(&job))
			{
				PRINTLN("loader thread: {} - got a MESH job", a_thread_id);
				assert(&mesh_job_processor != nullptr);
				// I suspect crash was because of this, threads raced here before MeshGenStorage constructor was finished -> no job processors created yer
				mesh_job_processor.process_job(*mesh_job); 
				// crashed again, so it seems it's because something else...
			}
			else if (auto vao_job = std::get_if<VAOLoadInfo>(&job))
			{
				PRINTLN("loader thread: {} - got a VAO job", a_thread_id);
				assert(&vaoinfo_job_processor != nullptr);
				vaoinfo_job_processor.process_job(*vao_job);
			}
			/// Now just add everything for textures to be processed and loaded etc...
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

RenderRequestReturner::RenderRequestReturner(ModelGenStorage& a_gen_storage) : renderreqret_gen_storage(a_gen_storage) {}

std::optional<RenderRequest> RenderRequestReturner::return_request(const hashed_string_64& a_request)
{
	{
		std::lock_guard<std::mutex> guard(renderreqret_gen_storage.mesh_mutex);
		auto& datas = renderreqret_gen_storage.mesh_datas;
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
		auto& datas = renderreqret_gen_storage.mesh_datas;
		size_t start_index = 0;
		size_t mesh_index = 0;
		bool found = false;
		std::lock_guard<std::mutex> guard(renderreqret_gen_storage.mesh_mutex);
		for (auto& request : a_request)
		{
			found = false;
			for (; mesh_index < datas.size(); mesh_index++)
			{
				if (request == datas[mesh_index].path_hashed)
				{
					render_requests.emplace_back(request, datas[mesh_index].VAO, (GLuint)datas[mesh_index].indices.size(), glm::mat4(1), 0, 0);
					found = true;
					start_index = mesh_index;
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

VAOLoadReturner::VAOLoadReturner(ModelGenStorage& a_gen_storage) : vaoreturner_gen_storage(a_gen_storage) {}

std::optional<VAOLoadRequest> VAOLoadReturner::return_request()
{
	return vaoreturner_gen_storage.vaoload_requests.get_message();
}

std::optional<std::vector<VAOLoadRequest>> VAOLoadReturner::return_requests()
{
	return vaoreturner_gen_storage.vaoload_requests.get_messages();
}