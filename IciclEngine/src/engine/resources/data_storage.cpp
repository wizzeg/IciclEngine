#include <engine/resources/data_storage.h>
#include <engine/utilities/macros.h>

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
				//data.path = data.path_hashed.string;
				data.ram_load_status = ELoadStatus::StartedLoad;
				start_load = true;
				sort_data(data_lock);
			}
		}
		if (start_load) // if we should load we load the mesh data from file
		{
			MeshData data = obj_parser.load_mesh_from_filepath(a_job.path_hashed.string);
			{
				std::unique_lock<std::mutex> data_lock(meshjob_gen_storage.mesh_mutex);
				if (data.ram_load_status == ELoadStatus::Loaded)
				{
					data.vao_load_status = ELoadStatus::RequestedLoad;
					meshjob_gen_storage.vaoload_requests.add_message(VAOLoadRequest{ data }); // copying data for the VAOLoadRequest
				}
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
			if (a_job.hashed_path == datas[i].path_hashed)
			{
				found_data = true;
				datas[i].vao_load_status = ELoadStatus::NotLoaded; /// I need the actual data too...
				break;
			}
		}
		if (!found_data)
		{
			MeshData new_mesh;
			new_mesh.path_hashed = a_job.hashed_path;
			//new_mesh.path = a_job.hashed_path.string;
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
				return mesh_a.path_hashed < mesh_b.path_hashed;
			});
	}
}

TexGenInfoProcessor::TexGenInfoProcessor(ModelGenStorage& a_gen_storage) : texgeninfo_gen_storage(a_gen_storage) {}

void TexGenInfoProcessor::process_job(TextureGenInfo& a_job)
{
	if (a_job.texture_gen_status == ELoadStatus::Loaded)
	{
		std::unique_lock<std::mutex> texture_lock(texgeninfo_gen_storage.texture_mutex);
		auto& datas = texgeninfo_gen_storage.texuture_datas;
		bool found_data = false;
		for (size_t i = 0; i < datas.size(); i++)
		{
			if (a_job.hashed_path == datas[i].hashed_path)
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
			new_texture.hashed_path = a_job.hashed_path;
			datas.push_back(new_texture);
			sort_data(texture_lock);
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
				return tex_a.hashed_path < tex_b.hashed_path;
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
				if (a_job.path_hashed == data.hashed_path)
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
				data.hashed_path = a_job.path_hashed;
				//data.path = a_job.path_hashed.string;
				load_texture = true;
				start_index = datas.size() - 1;
				sort_data(texture_lock);
				// handle generation of it
			}
		}
		if (load_texture)
		{
			TextureData new_data = texturedata_gen_storage.model_loader.load_texture_from_file(a_job.path_hashed.string.c_str());
			{
				std::unique_lock<std::mutex> texture_lock(texturedata_gen_storage.texture_mutex);
				auto& datas = texturedata_gen_storage.texuture_datas;
				start_index = std::max((size_t)0, std::min(datas.size() - 1, start_index));
				EDirection direction = EDirection::None;
				bool data_inserted = false;
				for (; start_index < datas.size();)
				{
					auto& data = datas[start_index];
					if (new_data.hashed_path == data.hashed_path)
					{
						data = new_data;
						data_inserted = true;
						break;
					}
					else if (direction == EDirection::None && new_data.hashed_path > data.hashed_path)
					{
						direction = EDirection::Down;
						start_index--;
					}
					else if (direction == EDirection::None && new_data.hashed_path < data.hashed_path)
					{
						direction = EDirection::Up;
						start_index++;
					}
					else if (direction == EDirection::Down && new_data.hashed_path > data.hashed_path) start_index--;
					else if (direction == EDirection::Up && new_data.hashed_path < data.hashed_path) start_index++;
					else if (direction == EDirection::Up && new_data.hashed_path > data.hashed_path) break; // it's not in here
					else if (direction == EDirection::Down && new_data.hashed_path < data.hashed_path) break; // it's not in here
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
				return tex_a.hashed_path < tex_b.hashed_path;
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
				if (a_request.mesh_hash == datas[i].path_hashed.hash)
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
				if (a_request.tex_hash == datas[i].hashed_path.hash)
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
	
	std::vector<RenderRequest> render_requests;
	render_requests.reserve(a_request.size());
	hashed_string_64 invalid_hash = hashed_string_64(" ");
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
				if (request.mesh_hash == data.path_hashed.hash)
				{
					render_requests.emplace_back
						(request.model_matrix, request.mesh_hash, request.tex_hash, data.VAO, (GLsizei)data.num_indicies, 0, 0);
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
				if (request.tex_hash == data.hashed_path.hash)
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
	}
	// sort for texture/material first, and then by mesh.. but later I need to sort by shader -> material -> mesh.
	std::sort(render_requests.begin(), render_requests.end(), []
	(const RenderRequest& request_a, const RenderRequest& request_b)
		{
			if (request_a.tex_hash != request_b.tex_hash)
				return request_a.tex_hash < request_b.tex_hash;
			return request_a.mesh_hash < request_b.mesh_hash;
		});
	return render_requests;
}

// I don't remember what this copy did... I thing I changed something... just too scared to change it
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
