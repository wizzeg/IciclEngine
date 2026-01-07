#include <engine/resources/data_storage.h>
#include <engine/utilities/macros.h>
#include <engine/utilities/utilities.h>

MeshJobProcessor::MeshJobProcessor(ModelGenStorage& a_gen_storage) : meshjob_gen_storage(a_gen_storage) {}
void MeshJobProcessor::process_job(MeshDataJob& a_job)
{
	if (a_job.request_type == ERequestType::LoadFromFile || a_job.request_type == ERequestType::ReloadFromFile)
	{
		bool found_data = false;
		bool start_load = false;
		{
			std::unique_lock <std::mutex> data_lock(meshjob_gen_storage.mesh_mutex);
			auto& datas = meshjob_gen_storage.mesh_datas;

			// look if data already exists
			for (size_t i = 0; i < datas.size(); i++)
			{
				if (a_job.path_hashed.hash == datas[i].hash)
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
				//if (!meshjob_gen_storage.mesh_map.contains(a_job.path_hashed.hash) 
				//	|| (meshjob_gen_storage.mesh_map.contains(a_job.path_hashed.hash) 
				//		&& meshjob_gen_storage.mesh_map[a_job.path_hashed.hash].ram_load_status != ELoadStatus::StartedLoad)
				//	|| (meshjob_gen_storage.mesh_map.contains(a_job.path_hashed.hash)
				//		&& meshjob_gen_storage.mesh_map[a_job.path_hashed.hash].ram_load_status != ELoadStatus::Loaded))
				//{
				//	meshjob_gen_storage.mesh_map[a_job.path_hashed.hash] = MeshData();
				//	meshjob_gen_storage.mesh_map[a_job.path_hashed.hash].contents->hashed_path = a_job.path_hashed;
				//	meshjob_gen_storage.mesh_map[a_job.path_hashed.hash].hash = a_job.path_hashed.hash;
				//	meshjob_gen_storage.mesh_map[a_job.path_hashed.hash].ram_load_status = ELoadStatus::StartedLoad;
				//}
				MeshData& data = meshjob_gen_storage.mesh_datas.back();
				data.contents->hashed_path = a_job.path_hashed;
				data.hash = a_job.path_hashed.hash;
				//data.path = data.path_hashed.string;
				data.ram_load_status = ELoadStatus::StartedLoad;
				start_load = true;
				sort_data(data_lock); /// don't use sort, just do this insertion manually!
			}
		}
		if (start_load) // if we should load we load the mesh data from file
		{
			MeshData data = obj_parser.load_mesh_from_filepath(a_job.path_hashed.string);
			{
				std::unique_lock<std::mutex> data_lock(meshjob_gen_storage.mesh_mutex);
				if (!meshjob_gen_storage.mesh_map.contains(a_job.path_hashed.hash)
					|| (meshjob_gen_storage.mesh_map.contains(a_job.path_hashed.hash)
						&& meshjob_gen_storage.mesh_map[a_job.path_hashed.hash].ram_load_status != ELoadStatus::StartedLoad)
					|| (meshjob_gen_storage.mesh_map.contains(a_job.path_hashed.hash)
						&& meshjob_gen_storage.mesh_map[a_job.path_hashed.hash].ram_load_status != ELoadStatus::Loaded))
				{
					meshjob_gen_storage.mesh_map[a_job.path_hashed.hash] = data;
					meshjob_gen_storage.mesh_map[a_job.path_hashed.hash].contents->hashed_path = a_job.path_hashed;
					meshjob_gen_storage.mesh_map[a_job.path_hashed.hash].hash = a_job.path_hashed.hash;
					meshjob_gen_storage.mesh_map[a_job.path_hashed.hash].ram_load_status = ELoadStatus::StartedLoad;
				}
				if (data.ram_load_status == ELoadStatus::Loaded)
				{
					data.vao_load_status = ELoadStatus::RequestedLoad;
					meshjob_gen_storage.vaoload_requests.add_message(VAOLoadRequest{ data }); // copying data for the VAOLoadRequest
				}
				auto& datas = meshjob_gen_storage.mesh_datas;
				bool found_data = false;       // Make sure that we add it, this should always exist, but safety check
				for (size_t i = 0; i < datas.size(); i++)
				{
					if (a_job.path_hashed.hash == datas[i].hash)
					{
						datas[i] = data;
						found_data = true;
						break;
					}
				}
				if (!found_data) // This should not happen, but we have to add it
				{
					datas.push_back(data);
					sort_data(data_lock); /// don't use sort, just do this insertion manually!
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
				return mesh_a.hash < mesh_b.hash;
			});
	}
}

VAOInfoProcessor::VAOInfoProcessor(ModelGenStorage& a_gen_storage) : vaoinfo_gen_storage(a_gen_storage){}

void VAOInfoProcessor::process_job(VAOLoadInfo& a_job)
{
	if (a_job.vao != 0)
	{
		std::unique_lock<std::mutex> data_lock(vaoinfo_gen_storage.mesh_mutex);
		if (vaoinfo_gen_storage.mesh_map.contains(a_job.hashed_path.hash))
		{
			vaoinfo_gen_storage.mesh_map[a_job.hashed_path.hash].vao_load_status = ELoadStatus::Loaded;
			vaoinfo_gen_storage.mesh_map[a_job.hashed_path.hash].VAO = a_job.vao;
			vaoinfo_gen_storage.mesh_map[a_job.hashed_path.hash].contents->EBO = a_job.ebo;
			vaoinfo_gen_storage.mesh_map[a_job.hashed_path.hash].contents->VBOs = a_job.VBOs;
		}
		auto& datas = vaoinfo_gen_storage.mesh_datas;
		bool found_data = false;
		for (size_t i = 0; i < datas.size(); i++)
		{
			if (a_job.hashed_path.hash == datas[i].hash)
			{
				auto& data = datas[i];
				found_data = true;
				data.vao_load_status = ELoadStatus::Loaded; /// I need the actual data too...
				data.VAO = a_job.vao;
				data.contents->EBO = a_job.ebo;
				data.contents->VBOs = a_job.VBOs;
				break;
			}
		}
		if (!found_data)
		{
			MeshData new_mesh;
			new_mesh.vao_load_status = ELoadStatus::Loaded;
			new_mesh.VAO = a_job.vao;
			new_mesh.contents->EBO = a_job.ebo;
			new_mesh.contents->VBOs = a_job.VBOs;
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
			if (a_job.hashed_path.hash == datas[i].hash)
			{
				found_data = true;
				datas[i].vao_load_status = ELoadStatus::NotLoaded; /// I need the actual data too...
				break;
			}
		}
		if (!found_data)
		{
			MeshData new_mesh;
			new_mesh.contents->hashed_path = a_job.hashed_path;
			new_mesh.hash = a_job.hashed_path.hash;
			new_mesh.vao_load_status = ELoadStatus::NotLoaded;
			datas.push_back(new_mesh);
			sort_data(data_lock);
		}
	}
}

void VAOInfoProcessor::sort_data(std::unique_lock<std::mutex>& a_lock)
{
	if (a_lock.owns_lock())
	{
		std::sort(vaoinfo_gen_storage.mesh_datas.begin(), vaoinfo_gen_storage.mesh_datas.end(), [](const MeshData& mesh_a, const MeshData& mesh_b)
			{
				return mesh_a.hash < mesh_b.hash;
			});
	}
}

TexGenInfoProcessor::TexGenInfoProcessor(ModelGenStorage& a_gen_storage) : texgeninfo_gen_storage(a_gen_storage) {}

void TexGenInfoProcessor::process_job(TextureGenInfo& a_job)
{
	if (a_job.texture_gen_status == ELoadStatus::Loaded)
	{
		std::unique_lock<std::mutex> texture_lock(texgeninfo_gen_storage.texture_mutex);
		if (texgeninfo_gen_storage.tex_map.contains(a_job.hashed_path.hash))
		{
			texgeninfo_gen_storage.tex_map[a_job.hashed_path.hash].texture_gen_status = a_job.texture_gen_status;
			texgeninfo_gen_storage.tex_map[a_job.hashed_path.hash].texture_id = a_job.texture_id;
		}
		auto& datas = texgeninfo_gen_storage.texuture_datas;
		bool found_data = false;
		for (size_t i = 0; i < datas.size(); i++)
		{
			if (a_job.hashed_path.hash == datas[i].hash)
			{
				auto& data = datas[i];
				found_data = true;
				data.texture_gen_status = a_job.texture_gen_status; /// I need the actual data too...
				data.texture_id = a_job.texture_id;
				break;
			}
		}
		if (!found_data)
		{
			TextureData new_texture;
			new_texture.texture_gen_status = a_job.texture_gen_status;
			new_texture.texture_id = a_job.texture_id;
			new_texture.contents->hashed_path = a_job.hashed_path;
			datas.push_back(new_texture);
			sort_data(texture_lock); /// don't use sort, just do this insertion manually!
		}
	}
}

void TexGenInfoProcessor::sort_data(std::unique_lock<std::mutex>& a_lock)
{
	if (a_lock.owns_lock())
	{
		std::sort(texgeninfo_gen_storage.texuture_datas.begin(), texgeninfo_gen_storage.texuture_datas.end(),
			[](const TextureData& tex_a, const TextureData& tex_b)
			{
				return tex_a.hash < tex_b.hash;
			});
	}
}



TextureDataProcessor::TextureDataProcessor(ModelGenStorage& a_gen_storage) : texturedata_gen_storage(a_gen_storage){}

void TextureDataProcessor::process_job(TextureDataJob& a_job)
{
	if (a_job.request_type == ERequestType::LoadFromFile || a_job.request_type == ERequestType::ReloadFromFile)
	{
		bool found_texture = false;
		bool load_texture = false;
		size_t start_index = 0;
		{
			std::unique_lock<std::mutex> texture_lock(texturedata_gen_storage.texture_mutex);
			auto& datas = texturedata_gen_storage.texuture_datas;
			for (size_t i = 0; i < datas.size(); i++)
			{
				auto& data = datas[i];
				if (a_job.path_hashed.hash == data.hash)
				{
					// found a match
					if (data.texture_ram_status == ELoadStatus::RequestedLoad || data.texture_ram_status == ELoadStatus::NotLoaded)
					{
						data.texture_ram_status = ELoadStatus::StartedLoad;
						start_index = i;
						load_texture = true;
						found_texture = true;
						break;
					}
					else if (data.texture_ram_status == ELoadStatus::Loaded || data.texture_ram_status == ELoadStatus::StartedLoad)
					{
						found_texture = true;
						break;
					}
				}
			}
			if (!found_texture)
			{
				datas.emplace_back();
				auto& data = datas.back();
				data.texture_ram_status = ELoadStatus::StartedLoad;
				data.contents->hashed_path = a_job.path_hashed;
				data.hash = a_job.path_hashed.hash;
				//data.path = a_job.path_hashed.string;
				load_texture = true;
				start_index = datas.size() - 1;
				sort_data(texture_lock); /// don't use sort, just do this insertion manually!
				// handle generation of it
			}
		}
		if (load_texture)
		{
			TextureData new_data = texturedata_gen_storage.model_loader.load_texture_from_file(a_job.path_hashed.string.c_str());
			{
				std::unique_lock<std::mutex> texture_lock(texturedata_gen_storage.texture_mutex);
				if (!texturedata_gen_storage.tex_map.contains(new_data.hash)
					|| (texturedata_gen_storage.tex_map.contains(new_data.hash)
						&& texturedata_gen_storage.tex_map[new_data.hash].texture_ram_status != ELoadStatus::StartedLoad)
					|| (texturedata_gen_storage.tex_map.contains(new_data.hash)
						&& texturedata_gen_storage.tex_map[new_data.hash].texture_ram_status != ELoadStatus::Loaded))
				{
					texturedata_gen_storage.tex_map[new_data.hash] = TextureData();
					texturedata_gen_storage.tex_map[new_data.hash].contents->hashed_path = a_job.path_hashed;
					texturedata_gen_storage.tex_map[new_data.hash].hash = a_job.path_hashed.hash;
					texturedata_gen_storage.tex_map[new_data.hash].texture_ram_status = ELoadStatus::StartedLoad;
				}
				else texturedata_gen_storage.tex_map[new_data.hash] = new_data;
				auto& datas = texturedata_gen_storage.texuture_datas;
				start_index = std::max((size_t)0, std::min(datas.size() - 1, start_index));
				EDirection direction = EDirection::None;
				bool data_inserted = false;
				for (; start_index < datas.size();)
				{
					auto& data = datas[start_index];
					if (new_data.contents->hashed_path == data.contents->hashed_path)
					{
						data = new_data;
						data_inserted = true;
						break;
					}
					else if (direction == EDirection::None && new_data.hash > data.hash)
					{
						direction = EDirection::Down;
						start_index--;
					}
					else if (direction == EDirection::None && new_data.hash < data.hash)
					{
						direction = EDirection::Up;
						start_index++;
					}
					else if (direction == EDirection::Down && new_data.hash > data.hash) start_index--;
					else if (direction == EDirection::Up && new_data.hash < data.hash) start_index++;
					else if (direction == EDirection::Up && new_data.hash > data.hash) break; // it's not in here
					else if (direction == EDirection::Down && new_data.hash < data.hash) break; // it's not in here
					else
					{
						PRINTLN("BROKE");
						break;
					}
				}
				if (!data_inserted)
				{
					datas.push_back(new_data);
					sort_data(texture_lock);
					data_inserted = true;
					start_index = datas.size() - 1;
				}
				if (data_inserted)
				{
					new_data.texture_gen_status = ELoadStatus::RequestedLoad;
					texturedata_gen_storage.texgen_requests.add_message(TexGenRequest{new_data});
					//sort_data(texture_lock);
				}
			}
		}
	}
}

void TextureDataProcessor::sort_data(std::unique_lock<std::mutex>& a_lock)
{
	if (a_lock.owns_lock())
	{
		std::sort(texturedata_gen_storage.texuture_datas.begin(), texturedata_gen_storage.texuture_datas.end(), [](const TextureData& tex_a, const TextureData& tex_b)
			{
				return tex_a.hash < tex_b.hash;
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
	: GenStorageThreadPool(num_threads), mesh_job_processor(*this), vaoinfo_job_processor(*this), render_request_returner(*this), 
	vaoload_returner(*this), tex_job_processor(*this), texgen_returner(*this), tex_geninfo_processor(*this)
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
				PRINTLN("loader thread: {} - got a VAO info job", a_thread_id);
				assert(&vaoinfo_job_processor != nullptr);
				vaoinfo_job_processor.process_job(*vao_job);
			}
			else if (auto tex_job = std::get_if<TextureDataJob>(&job))
			{
				PRINTLN("loader thread: {} - got a TEX job", a_thread_id);
				assert(&tex_job_processor != nullptr);
				tex_job_processor.process_job(*tex_job);
			}
			else if (auto texgen_job = std::get_if<TextureGenInfo>(&job))
			{
				PRINTLN("loader thread: {} - got a TEX gen info job", a_thread_id);
				assert(&tex_geninfo_processor != nullptr);
				tex_geninfo_processor.process_job(*texgen_job);
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

std::optional<RenderRequest> RenderRequestReturner::return_request(const PreRenderRequest& a_request)
{
	{
		RenderRequest render_request;
		bool found_mesh = false;
		{
			std::lock_guard<std::mutex> guard(renderreqret_gen_storage.mesh_mutex);
			auto& datas = renderreqret_gen_storage.mesh_datas;
			for (size_t i = 0; i < datas.size(); i++)
			{
				if (a_request.mesh_hash == datas[i].hash)
				{
					render_request = RenderRequest
						(glm::mat4(1), a_request.mesh_hash, a_request.tex_hash, datas[i].VAO, (GLsizei)datas[i].num_indicies, 0, 0);
					found_mesh = true;
					break;
				}
			}
		}
		if (found_mesh)
		{
			std::lock_guard<std::mutex> guard(renderreqret_gen_storage.texture_mutex);
			auto& datas = renderreqret_gen_storage.texuture_datas;
			for (size_t i = 0; i < datas.size(); i++)
			{
				if (a_request.tex_hash == datas[i].hash)
				{
					render_request.material_id = datas[i].texture_id;
					return render_request;
				}
			}
		}
	}
	return std::nullopt;
}


std::optional<std::vector<RenderRequest>> RenderRequestReturner::return_requests(std::vector<PreRenderRequest>& a_request, bool sorted) // does not require pre-sorting
{
	//HighResolutionTimer timer;
	//timer.start();
	// so the complexity is about tm < 5n log n.
	// so if 5n log n is greater than tm, then use the current method... Otherwise use this one.
	//
	// n log n (30 cycles) + n (6 cycles) extra sort
	// (t x m) (6 cycles)                 extra iterations
	// feels like extra itterations should almost always win...

	// perhaps something like... use extra sort if this holds (30 * n * log_2(n) < 6 * t* m) -> (5 * n * log_2(n) <  t * m)
	// this really just needs testing I guess... it completely depends on how many cycles the extra sort is.
	// I think I probably underestimated cost of iteration, it's more complex now... likely 45 cycles per full iteration?
	// tex iteration is more like 8 per miss ... around 17 for mesh loops ... setting stuff is around mesh 11, tex 6

	// so..  (n log n)(30) + n(6) vs (t(8) * m(17)) + n(17) -> (n log n)(30) vs (t(8) * m(17)) + n(11)
	// so use extra sort if (n log n)(30) < (t(8) * m(17)) + n(11)
	std::vector<RenderRequest> render_requests;
	render_requests.reserve(a_request.size());
	hashed_string_64 invalid_hash;
	bool use_map = false;
	bool sort_before_map = false;
	bool use_extra_sort = true;

	if (!use_map)
	{
		//if (a_request.size() * std::log(a_request.size())
		//	< (renderreqret_gen_storage.texuture_datas.size()* 8 + renderreqret_gen_storage.mesh_datas.size()*17 + a_request.size() * 11))
		if (use_extra_sort)
		{
			std::sort(a_request.begin(), a_request.end(), []
			(const PreRenderRequest& request_a, const PreRenderRequest& request_b)
				{
					return request_a.mesh_hash < request_b.mesh_hash;
				});
			std::lock_guard<std::mutex> mesh_guard(renderreqret_gen_storage.mesh_mutex);
			std::lock_guard<std::mutex> tex_guard(renderreqret_gen_storage.texture_mutex);
			auto& tex_datas = renderreqret_gen_storage.texuture_datas;
			auto& mesh_datas = renderreqret_gen_storage.mesh_datas;
			size_t mesh_index = 0;
			size_t mesh_start_index = 0;
			bool mesh_found = false;
			for (auto& request : a_request)
			{
				mesh_found = false;
				if (request.mesh_hash == invalid_hash.hash)
				{
					continue;
				}
				for (;mesh_index < mesh_datas.size(); mesh_index++)
				{
					auto& data = mesh_datas[mesh_index];
					if (request.mesh_hash == data.hash)
					{
						render_requests.emplace_back
							(request.model_matrix, request.mesh_hash, request.tex_hash, data.VAO, data.num_indicies, 0, 0);
						mesh_start_index = mesh_index;
						mesh_found = true;
						break;
					}
				}
				if (!mesh_found)
				{
					mesh_index = mesh_start_index;
				}
			}

			std::sort(render_requests.begin(), render_requests.end(), []
			(const RenderRequest& request_a, const RenderRequest& request_b)
				{
					return request_a.tex_hash < request_b.tex_hash;
				});

			size_t tex_index = 0;
			size_t tex_start_index = 0;
			bool tex_found = false;
			for (auto& request : render_requests)
			{
				tex_found = false;
				if (request.tex_hash == invalid_hash)
				{
					request.material_id = 0;
					continue;
				}
				for (; tex_index < tex_datas.size(); tex_index++)
				{
					auto& data = tex_datas[tex_index];
					if (request.tex_hash == data.hash)
					{
						request.material_id = data.texture_id;
						tex_start_index = tex_index;
						tex_found = true;
						break;
					}
							
				}
				if (!tex_found)
				{
					tex_index = tex_start_index;
				}
			}
			//PRINTLN("used extra sort");
		}
		else
		{
			std::sort(a_request.begin(), a_request.end(), []
			(const PreRenderRequest& request_a, const PreRenderRequest& request_b)
				{
					if (request_a.mesh_hash == request_b.mesh_hash)
						return request_a.tex_hash < request_b.tex_hash;
					return request_a.mesh_hash < request_b.mesh_hash;
					
				});
			std::lock_guard<std::mutex> mesh_guard(renderreqret_gen_storage.mesh_mutex);
			std::lock_guard<std::mutex> tex_guard(renderreqret_gen_storage.texture_mutex);
			auto& tex_datas = renderreqret_gen_storage.texuture_datas;
			auto& mesh_datas = renderreqret_gen_storage.mesh_datas;
			size_t mesh_index = 0;
			size_t mesh_start_index = 0;
			bool mesh_found = false;
			size_t tex_index = 0;
			size_t tex_start_index = 0;
			GLuint tex_id = 0;
			GLuint vao = 0;
			GLsizei size = 0;
			bool tex_found = false;
			uint64_t prev_mesh_hash = invalid_hash.hash;
			bool requested_mesh = false;
			bool requested_tex = false;
			for (auto& request : a_request)
			{
				tex_id = 0;
				vao = 0;
				size = 0;
				mesh_found = false;
				if (request.mesh_hash == invalid_hash.hash)
					continue;
				if (prev_mesh_hash != request.mesh_hash)
				{
					tex_index = 0;
					tex_start_index = 0;
				}
					
				for (; mesh_index < mesh_datas.size(); mesh_index++) // should check mesh/tex if it's loaded, if not, add it as load job.
				{
					MeshData& mesh_data = mesh_datas[mesh_index];
					if (request.mesh_hash == mesh_data.hash)
					{
						vao = mesh_data.VAO;
						size = mesh_data.num_indicies;

						mesh_start_index = mesh_index;
						prev_mesh_hash = mesh_data.hash;
						mesh_found = true;
						tex_index = 0;
						tex_found = false;
						if (request.tex_hash != invalid_hash.hash)
						{
							for (; tex_index < tex_datas.size(); tex_index++)
							{
								TextureData& tex_data = tex_datas[tex_index];
								if (request.tex_hash == tex_data.hash)
								{
									tex_id = tex_data.texture_id;

									tex_start_index = tex_index;
									tex_found = true;
									break;
								}
							}
						}
						if (!tex_found)
							tex_index = tex_start_index;

						break;
					}
				}
				if (!mesh_found)
				{
					mesh_index = mesh_start_index;
				}

				else
				{
					render_requests.emplace_back
					(request.model_matrix, request.mesh_hash, request.tex_hash, vao, size, 0, tex_id);
				}
			}
		}
	}
	else
	{
		if (sort_before_map)
		{
			std::sort(a_request.begin(), a_request.end(), []
			(const PreRenderRequest& request_a, const PreRenderRequest& request_b)
				{
					if (request_a.mesh_hash == request_b.mesh_hash)
						return request_a.tex_hash < request_b.tex_hash;
					return request_a.mesh_hash < request_b.mesh_hash;

				});
		}
		for (auto& request : a_request)
		{
			RenderRequest new_request;
			MeshData& mesh = renderreqret_gen_storage.mesh_map[request.mesh_hash];
			//new_request.indices_size = mesh.num_indicies;
			//new_request.vao = mesh.VAO;
			//new_request.tex_hash = request.tex_hash;
			//new_request.mesh_hash = request.mesh_hash;
			TextureData& tex = renderreqret_gen_storage.tex_map[request.tex_hash];
			//new_request.material_id = tex.texture_id;
			//new_request.model_matrix = request.model_matrix;
			render_requests.emplace_back
			(request.model_matrix, request.mesh_hash, request.tex_hash, mesh.VAO, mesh.num_indicies, 0 , tex.texture_id);
		}
		//for (auto& request : render_requests)
		//{
		//	//RenderRequest new_request;
		//	//MeshData& mesh = renderreqret_gen_storage.mesh_map[request.mesh_hash];
		//	//new_request.indices_size = mesh.num_indicies;
		//	//new_request.vao = mesh.VAO;
		//	TextureData& tex = renderreqret_gen_storage.tex_map[request.tex_hash];
		//	request.material_id = tex.texture_id;
		//	request.model_matrix = request.model_matrix;
		//}
	}
	
	// sort for texture/material first, and then by mesh.. but later I need to sort by shader -> material -> mesh.
	std::sort(render_requests.begin(), render_requests.end(), []
	(const RenderRequest& request_a, const RenderRequest& request_b)
		{
			if (request_a.tex_hash == request_b.tex_hash)
				return request_a.mesh_hash < request_b.mesh_hash;
			return request_a.tex_hash < request_b.tex_hash;
		});
	//timer.stop();
	//PRINTLN("render requests took: {}", timer.get_time_ms());
	return render_requests;
}
//
//std::optional<std::vector<RenderRequest>> RenderRequestReturner::return_requests(std::vector<PreRenderRequest>& a_request, bool sorted) // does not require pre-sorting
//{
//	// should add the linear search too, where I loop over mesh or material/texture for every hit on the request hit.
//	// but that's only reasonable if mesh and material/texture is small, otherwise it'll be terrible.
//	// (m + t) < n/2 or log n < (~15) * t -> so if n/t < 200? do the linear search, looping over material/mesh over and over for each request, one sort less
//	// (m + t) > n/2 or log n > (~15) * t -> so if n/t > 200? use the merge join with one extra sort.
//	// hmm, I think it'd be very very rare for extra sort to win out...or something, this is hard to think about
//	// I think so, sorting is extremely expensive after all...
//	std::vector<RenderRequest> render_requests;
//	render_requests.reserve(a_request.size());
//	hashed_string_64 invalid_hash = hashed_string_64(" ");
//	{
//		std::sort(a_request.begin(), a_request.end(), []
//		(const PreRenderRequest& request_a, const PreRenderRequest& request_b)
//			{
//				return request_a.mesh_hash < request_b.mesh_hash;
//			});
//		std::lock_guard<std::mutex> mesh_guard(renderreqret_gen_storage.mesh_mutex);
//		std::lock_guard<std::mutex> tex_guard(renderreqret_gen_storage.texture_mutex);
//		auto& tex_datas = renderreqret_gen_storage.texuture_datas;
//		auto& mesh_datas = renderreqret_gen_storage.mesh_datas;
//		size_t mesh_index = 0;
//		size_t mesh_start_index = 0;
//		bool mesh_found = false;
//		for (auto& request : a_request)
//		{
//			mesh_found = false;
//			if (request.mesh_hash == invalid_hash.hash)
//			{
//				continue;
//			}
//			for (;mesh_index < mesh_datas.size(); mesh_index++)
//			{
//				auto& data = mesh_datas[mesh_index];
//				if (request.mesh_hash == data.hash)
//				{
//					render_requests.emplace_back
//						(request.model_matrix, request.mesh_hash, request.tex_hash, data.VAO, (GLsizei)data.num_indicies, 0, 0);
//					mesh_start_index = mesh_index;
//					mesh_found = true;
//					break;
//				}
//			}
//			if (!mesh_found)
//			{
//				mesh_index = mesh_start_index;
//			}
//		}
//		std::sort(render_requests.begin(), render_requests.end(), []
//		(const RenderRequest& request_a, const RenderRequest& request_b)
//			{
//				return request_a.tex_hash < request_b.tex_hash;
//			});
//		size_t tex_index = 0;
//		size_t tex_start_index = 0;
//		bool tex_found = false;
//		for (auto& request : render_requests)
//		{
//			tex_found = false;
//			if (request.tex_hash == invalid_hash)
//			{
//				request.material_id = 0;
//				continue;
//			}
//			for (; tex_index < tex_datas.size(); tex_index++)
//			{
//				auto& data = tex_datas[tex_index];
//				if (request.tex_hash == data.hash)
//				{
//					request.material_id = data.texture_id;
//					tex_start_index = tex_index;
//					tex_found = true;
//					break;
//				}
//				
//			}
//			if (!tex_found)
//			{
//				tex_index = tex_start_index;
//			}
//		}
//	}
//	// sort for texture/material first, and then by mesh.. but later I need to sort by shader -> material -> mesh.
//	std::sort(render_requests.begin(), render_requests.end(), []
//	(const RenderRequest& request_a, const RenderRequest& request_b)
//		{
//			if (request_a.tex_hash != request_b.tex_hash)
//				return request_a.tex_hash < request_b.tex_hash;
//			return request_a.mesh_hash < request_b.mesh_hash;
//		});
//	return render_requests;
//}

// I don't remember what this copy did... I thing I changed something... just too scared to remove it
// this doesn't really need to be optional... but now it is.
//std::optional<std::vector<RenderRequest>> RenderRequestReturner::return_requests(std::vector<PreRenderRequest>& a_request, bool sorted) // does not require pre-sorting
//{
//	std::vector<RenderRequest> render_requests;
//	render_requests.reserve(a_request.size());
//	hashed_string_64 invalid_hash = hashed_string_64("invalidhash");
//	{
//		// lock both, we don't want any interruption
//		std::lock_guard<std::mutex> mesh_guard(renderreqret_gen_storage.mesh_mutex);
//		std::lock_guard<std::mutex> tex_guard(renderreqret_gen_storage.texture_mutex);
//		auto& tex_datas = renderreqret_gen_storage.texuture_datas;
//		auto& mesh_datas = renderreqret_gen_storage.mesh_datas;
//
//
//		/// move this sort to beginning, and change some bool for priority so that others go into a condition variable wait until priority is off
//		// meanwhile this sorts, and hopefully anything that was working on texture/mesh_datas will finish within time to not delay this action
//		std::sort(a_request.begin(), a_request.end(), []
//		(const PreRenderRequest& request_a, const PreRenderRequest& request_b)
//			{
//				return request_a.texture_hashed_path < request_b.texture_hashed_path;
//			});
//		size_t tex_index = 0;
//		size_t tex_start_index = 0;
//		bool tex_found = false;
//		for (auto& request : a_request)
//		{
//			tex_found = false;
//			if (request.texture_hashed_path == invalid_hash)
//			{
//				render_requests.emplace_back
//				(request.mesh_hashed_path, 0, 0, request.model_matrix, 0, 0, invalid_hash);
//				continue;
//			}
//			for (; tex_index < tex_datas.size(); tex_index++)
//			{
//				auto& data = tex_datas[tex_index];
//				if (request.texture_hashed_path == data.hashed_path)
//				{
//					GLuint id = data.texture_gen_status == ELoadStatus::Loaded ? data.texture_id : 0;
//					render_requests.emplace_back
//					(request.mesh_hashed_path, 0, 0, request.model_matrix, 0, id, request.texture_hashed_path);
//					tex_start_index = tex_index;
//					tex_found = true;
//					break;
//				}
//			}
//			if (!tex_found)
//			{
//				render_requests.emplace_back
//				(request.mesh_hashed_path, 0, 0, request.model_matrix, 0, 0, invalid_hash);
//				tex_index = tex_start_index;
//			}
//		}
//		std::sort(render_requests.begin(), render_requests.end(), []
//		(const RenderRequest& request_a, const RenderRequest& request_b)
//			{
//				return request_a.mesh_hashed_path < request_b.mesh_hashed_path;
//			});
//		size_t mesh_index = 0;
//		size_t mesh_start_index = 0;
//		bool mesh_found = false;
//		for (auto& request : render_requests)
//		{
//			mesh_found = false;
//			if (request.mesh_hashed_path == invalid_hash) continue;
//			for (; mesh_index < mesh_datas.size(); mesh_index++)
//			{
//				auto& data = mesh_datas[mesh_index];
//				if (request.mesh_hashed_path == data.path_hashed)
//				{
//					request.vao = data.VAO;
//					request.indices_size = (GLsizei)data.indices.size();
//					mesh_start_index = mesh_index;
//					mesh_found = true;
//					break;
//				}
//
//			}
//			if (!mesh_found)
//			{
//				mesh_index = mesh_start_index;
//			}
//		}
//
//	}
//	return render_requests;
//
//}

VAOLoadRequester::VAOLoadRequester(ModelGenStorage& a_gen_storage) : vaoload_gen_storage(a_gen_storage) {}

std::optional<VAOLoadRequest> VAOLoadRequester::return_request()
{
	return vaoload_gen_storage.vaoload_requests.get_message();
}

std::optional<std::vector<VAOLoadRequest>> VAOLoadRequester::return_requests()
{
	return vaoload_gen_storage.vaoload_requests.get_messages();
}

TexGenRequester::TexGenRequester(ModelGenStorage& a_gen_storage) : texgen_gen_storage(a_gen_storage) {}

std::optional<TexGenRequest> TexGenRequester::return_request()
{
	return texgen_gen_storage.texgen_requests.get_message();
}

std::optional<std::vector<TexGenRequest>> TexGenRequester::return_requests()
{
	return texgen_gen_storage.texgen_requests.get_messages();
}
