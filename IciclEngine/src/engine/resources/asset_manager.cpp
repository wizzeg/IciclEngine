#include <engine/resources/asset_manager.h>
#include <engine/utilities/macros.h>
#include <algorithm>
#include <chrono>
#include <engine/renderer/shader_loader.h>
#include <engine/resources/model_loader.h>

void AssetJobThread::process_load_mesh_job(MeshDataJob& a_job)
{
	{
		auto& meshes = asset_storage.mesh_map;
		auto job_hash = a_job.path_hashed.hash;
		std::unique_lock<std::mutex> mesh_lock(asset_storage.mesh_mutex);
		bool load_new_mesh = false;

		if (meshes.contains(job_hash))
		{
			if (meshes[job_hash]->modified_time <= a_job.job_time)
			{
				// exists so we'll figure out what to do with it
				if (meshes[job_hash]->ram_load_status == ELoadStatus::FailedLoadNoSpace)
				{
					load_new_mesh = true;
					meshes[job_hash]->ram_load_status = ELoadStatus::StartedLoad;
				}
			}
			else return;
		}
		else
		{
			// doesn't exist, so we must add it and then start working on it
			auto new_mesh = std::make_shared<MeshData>();
			new_mesh->hash = job_hash;
			new_mesh->ram_load_status = ELoadStatus::StartedLoad;
			new_mesh->modified_time = a_job.job_time;
			load_new_mesh = true;
			
			meshes[job_hash] = new_mesh;
		}
		mesh_lock.unlock();

		if (load_new_mesh)
		{
			auto new_mesh = std::make_shared<MeshData>(ObjParser::load_mesh_from_filepath(a_job.path_hashed.string));
			new_mesh->modified_time = a_job.job_time;
			if (new_mesh->ram_load_status == ELoadStatus::FailedLoadNoSpace)
			{
				AssetJob new_job = MeshDataJob(a_job);
				mesh_lock.lock();
				if (meshes.contains(job_hash))
				{
					// TODO: figure out what to do if it was deleted
					if (meshes[job_hash]->modified_time <= a_job.job_time)
					{
						meshes[job_hash]->ram_load_status = ELoadStatus::FailedLoadNoSpace;
						meshes[job_hash]->modified_time = a_job.job_time;
						asset_messages.job_queue.add_message(std::move(new_job));
					}
				}
				else
				{
					meshes[job_hash] = new_mesh;
					meshes[job_hash]->ram_load_status = ELoadStatus::FailedLoadNoSpace;
					asset_messages.job_queue.add_message(std::move(new_job));
				}
				mesh_lock.unlock();

			}
			else if (new_mesh->ram_load_status == ELoadStatus::Loaded)
			{
				mesh_lock.lock();
				if (meshes.contains(job_hash))
				{
					if (meshes[job_hash]->modified_time <= a_job.job_time)
					{
						new_mesh->vao_load_status = ELoadStatus::RequestedLoad;
						meshes[job_hash] = new_mesh;
						meshes[job_hash]->modified_time = a_job.job_time;
						VAOLoadRequest vao_req(*new_mesh.get());
						asset_messages.vao_queue.add_message(vao_req);
					}
				}
				else
				{
					new_mesh->vao_load_status = ELoadStatus::RequestedLoad;
					meshes[job_hash] = new_mesh;
					meshes[job_hash]->modified_time = a_job.job_time;
					VAOLoadRequest vao_req(*new_mesh.get());
					asset_messages.vao_queue.add_message(vao_req);
				}
				mesh_lock.unlock();
			}
		}
	}
}

void AssetJobThread::process_tex_job(TextureDataJob& a_job)
{
	{
		// check if the tex_job is already loaded
		// if not, then load it
		// always check all dependencies to ensure they're given the information they need
		std::unique_lock<std::mutex> tex_lock(asset_storage.tex_mutex);
		uint64_t job_hash = a_job.path_hashed.hash;
		auto& texs = asset_storage.tex_map;
		bool start_load = true;
		bool fulfill_deps = false;
		if (a_job.request_type == ERequestType::LoadFromFile)
		{
			if (texs.contains(job_hash))
			{
				auto& tex = texs[job_hash];
				if (tex->modified_time > a_job.job_time)
				{
					return;
				}
				switch (tex->texture_ram_status)
				{
				case ELoadStatus::NotLoaded:
					break;
				case ELoadStatus::FailedLoadNoSpace:
					break;
				case ELoadStatus::Loaded:
					start_load = false;
					fulfill_deps = true;
					return;
				default:
					start_load = false;
					break;
				}
				// we got the hash, have to check what's up with it...
			}
			else start_load = true;
			if (start_load)
			{
				// ok we load mat.
				auto new_tex = std::make_shared<TextureData>();
				new_tex->texture_ram_status = ELoadStatus::StartedLoad;
				new_tex->hash = job_hash;
				new_tex->contents->hashed_path = a_job.path_hashed;
				new_tex->modified_time = a_job.job_time;
				texs[job_hash] = new_tex;
				tex_lock.unlock();

				new_tex = std::make_shared<TextureData>(ModelLoader::load_texture_from_file(a_job.path_hashed.string));

				tex_lock.lock();
			}
		}

		// now enter all the dependencies
		if (fulfill_deps)
		{
			ValidateMatDependencies dep_job(a_job.path_hashed, EDependecyType::Texture, false, a_job.job_time);
			asset_messages.job_queue.add_message(std::move(dep_job));
		}
	}
}

void AssetJobThread::process_shader_job(ShaderDataJob& a_job)
{
	{
		uint64_t job_hash = a_job.path_hashed.hash;
		std::unique_lock<std::mutex> shader_lock(asset_storage.shader_mutex);
		auto& shaders = asset_storage.shader_map;
		bool start_load = true;
		bool fulfill_deps = false;
		if (shaders.contains(job_hash))
		{
			// has shader... now what?
			auto& shader = shaders[job_hash];
			if (shader->modified_time > a_job.job_time)
			{
				return;
			}
			switch (shader->loading_status)
			{
			case ELoadStatus::NotLoaded:
				break;
			case ELoadStatus::FailedLoadNoSpace:
				break;
			case ELoadStatus::ShaderLoadedProgram:
				start_load = false;
				fulfill_deps = true;
				return;
			default:
				start_load = false;
				break;
			}
		}
		else start_load = true;
		if (start_load)
		{
			auto new_shader = std::make_shared<ShaderData>();
			new_shader->hashed_path = a_job.path_hashed;
			new_shader->loading_status = ELoadStatus::StartedLoad;
			new_shader->modified_time = a_job.job_time;

			shader_lock.unlock();

			ShaderData temp_shader = ShaderLoader::load_shader_from_path(a_job.path_hashed.string);
			new_shader = std::make_shared<ShaderData>(temp_shader);

			shader_lock.lock();
			if (shaders[job_hash]->modified_time <= a_job.job_time)
			{
				shaders[job_hash] = new_shader;
			}
			else return;
			ProgramLoadRequest shader_req(temp_shader);
			asset_messages.program_queue.add_message(std::move(shader_req));
			shader_lock.unlock();
		}
		if (fulfill_deps)
		{
			ValidateMatDependencies dep_job(a_job.path_hashed, EDependecyType::Shader, false, a_job.job_time);
			asset_messages.job_queue.add_message(std::move(dep_job));
		}
	}
}

void AssetJobThread::process_mat_job(MeshDataJob& a_job)
{
	//TODO: check with the modified_time and job_time so that work isn't perforemd out of order.
	//TODO: handle cases for different load statuses

	// lock, materials -> shaders/textures -> dependencies
	// we get a material we wanna load
	// check if it exists first.
	{
		std::shared_ptr<MaterialData> new_mat;
		std::unique_lock<std::mutex> mat_lock(asset_storage.mat_mutex);
		auto& materials = asset_storage.mat_map;
		auto mat_hash = a_job.path_hashed.hash;
		bool start_load = false;

		hashed_string_64 invalid_hash;
		hashed_string_64 shader_path;
		if (materials.contains(mat_hash))
		{
			//TODO figure out what to do if it was deleted or somethine
			// keep in mind it could've been pushed here even if it's just waiting for textures etc

			// check loading statuses etc

			// when materials delete themselves, they must remove themselves from all dependencies
			// also then remove the entry... or mark it as unloaded
			// if it is the latest to remove a dependency, so texture/shader has zero references, then remove tex/shdr
			if (materials[mat_hash]->modified_time > a_job.job_time)
			{
				// skip this job, this job is too late.
				return;
			}
			else if (materials[mat_hash]->load_status == ELoadStatus::NotLoaded) // need to handle all cases
			{
				// now we deal with that it's beend deleted before
				start_load = true;

			}
			else if (materials[mat_hash]->load_status == ELoadStatus::MaterialDependenciesLoading)
			{
				return;
			}
			else // have to check if it's in the middle of deleting
			{
				//shader_path = materials[mat_hash]->shader_path;
				//new_mat = materials[mat_hash];
				return;
				// so we had it already... should we do more?.. probably not
			}
			
		}
		else start_load = true;
		if (start_load)
		{
			new_mat = std::make_shared<MaterialData>();
			new_mat->load_status = ELoadStatus::MaterialDependenciesLoading; // claimed for self
			new_mat->hashed_path = a_job.path_hashed;
			new_mat->modified_time = a_job.job_time;
			materials[mat_hash] = new_mat;
			mat_lock.unlock();
			bool load_shader = true;

			new_mat = std::make_shared<MaterialData>(ShaderLoader::load_material_from_path(a_job.path_hashed.string));
			auto& shaders = asset_storage.shader_map;
			auto& shader_dep = asset_storage.mat_shader_dependencies;
			shader_path = new_mat->shader_path;
			mat_lock.lock();
			materials[mat_hash] = new_mat;
			std::unique_lock<std::mutex> dep_shader_lock(asset_storage.dep_shader_mutex);

			// so if we have the shader, then it SHOULD have added itself

			// this is all we need to do for this... then we just push an assetjob for the shader path.
			if (shader_dep.contains(shader_path.hash)) // check so it has added it's dependencies
			{
				// the dependency is already added
				std::vector<uint64_t>& deps = shader_dep[shader_path.hash];
				bool registered = false;
				for (auto& dep : deps)
				{
					if (new_mat->hashed_path.hash == dep)
					{
						PRINTLN("unexpectedly the material was already added to shader dependencies!");
						registered = true;
						break;
					}
				}
				if (!registered)
				{
					// this should generally happen
					deps.push_back(new_mat->hashed_path.hash);
				}

			}
			else
			{
				shader_dep[shader_path.hash] = {new_mat->hashed_path.hash};
			}
			ShaderDataJob shader_job(shader_path, ERequestType::LoadFromFile, a_job.job_time);
			asset_messages.job_queue.add_message(shader_job);

			dep_shader_lock.unlock();

			//if (shaders.contains(new_mat->shader_path.hash))
			//{
			//	auto& shader = shaders[new_mat->shader_path.hash];
			//	
			//	// TODO: check if it's loaded, since in that case we'd have to add all the dependency stuff
			//	// TODO: handle timing stuff with modified time and job time
			//	// 
			//	switch (shader->loading_status) // I think something like this
			//	{
			//	case ELoadStatus::NotLoaded:
			//		load_shader = true;
			//		break;
			//	case ELoadStatus::Loaded:
			//		new_mat->gl_program = shader->gl_program;
			//		load_shader = false;
			//		break;
			//	default:
			//		load_shader = false;
			//		break;
			//	}
			//	// we've already started loading the shader
			//	if (shader_dep.contains(new_mat->shader_path.hash)) // check so it has added it's dependencies
			//	{
			//		// the dependency is already added
			//		std::vector<uint64_t>& deps = shader_dep[new_mat->shader_path.hash];
			//		bool registered = false;
			//		for (auto& dep : deps)
			//		{
			//			if (new_mat->hashed_path.hash == dep)
			//			{
			//				PRINTLN("unexpectedly the material was already added to shader dependencies!");
			//				registered = true;
			//				break;
			//			}
			//		}
			//		if (!registered)
			//		{
			//			// this should generally happen
			//			deps.push_back(new_mat->hashed_path.hash);
			//		}
			//			
			//	}
			//	else
			//	{
			//		PRINTLN("Shader added but not dependency!!!");
			//		// handle this logic issue
			//	}
			//	dep_shader_lock.unlock();
			//	shader_lock.unlock();
			//}
			//if (load_shader)
			//{
			//	// create temporary shader to mark existance and that we've started working on it
			//	std::shared_ptr<ShaderData> temp_shader = std::make_shared<ShaderData>();
			//	temp_shader->hashed_path = new_mat->shader_path;
			//	temp_shader->loading_status = ELoadStatus::StartedLoad;
			//	shaders[new_mat->shader_path.hash] = temp_shader;

			//	if (shader_dep.contains(new_mat->shader_path.hash)) // check so it has added it's dependencies
			//	{
			//		PRINTLN("Dependency for the shader should not yet exist! (unless it's been deleted before)");
			//		// the dependency is already added, should not be... handle this situation
			//		// or it could be if it's been deleted before!
			//		std::vector<uint64_t>& deps = shader_dep[new_mat->shader_path.hash];
			//		bool registered = false;
			//		for (auto& dep : deps)
			//		{
			//			if (new_mat->hashed_path.hash == dep)
			//			{
			//				PRINTLN("unexpectedly the material was already added to shader dependencies!");
			//				registered = true;
			//				break;
			//			}
			//		}
			//		if (!registered)
			//		{
			//			// this should generally happen
			//			deps.push_back(new_mat->hashed_path.hash);
			//		}
			//	}
			//	else
			//	{
			//		std::vector<uint64_t>& deps = shader_dep[new_mat->shader_path.hash];
			//		bool registered = false;
			//		for (auto& dep : deps)
			//		{
			//			if (new_mat->hashed_path.hash == dep)
			//			{
			//				PRINTLN("unexpectedly the material was already added to shader dependencies!");
			//				registered = true; 
			//				break;
			//			}
			//		}
			//		if (!registered)
			//		{
			//			// this should generally happen
			//			deps.push_back(new_mat->hashed_path.hash);
			//		}
			//		// handle this logic issue
			//	}
			//	dep_shader_lock.unlock();
			//	shader_lock.unlock();
			//	// we'll now load the shader, then later deal with the textures.

			//	ShaderData new_shader = ShaderLoader::load_shader_from_path(shader_path.string);
			//	new_shader.modified_time = a_job.job_time;
			//	shader_lock.lock();
			//	// replace the loaded shader... then we have to send it to gpu upload
			//	shaders[shader_path.hash] = std::make_shared<ShaderData>(new_shader);
			//	asset_messages.program_queue.add_message(ProgramLoadRequest{new_shader});
			//	shader_lock.unlock();
			//}

			// done with the shader dependency
			// now to do texture loading jobs etc.

			// for each texture, check if it's loaded and or GPU loaded...

		}

		//now we must register all dependencies, and start jobs.
		std::unique_lock<std::mutex> dep_tex_lock(asset_storage.dep_tex_mutex);

		std::type_index string_type = typeid(std::string);
		auto& tex_deps = asset_storage.mat_tex_dependencies;
 		for (auto& uniform : new_mat->uniforms)
		{
			if (auto uniform_string = std::get_if<std::string>(&uniform.value))
			{
				bool registered = false;
				hashed_string_64 tex_path(uniform_string->c_str());
				new_mat->tex_deps.emplace_back(tex_path.hash, false);
				if (tex_deps.contains(tex_path.hash))
				{
					for (auto& hash : tex_deps[tex_path.hash])
					{
						if (hash == mat_hash)
						{
							registered = true;
							break;
						}
					}
					if (!registered)
					{
						tex_deps[tex_path.hash].push_back(mat_hash);
					}
				}
				else
				{
					tex_deps[tex_path.hash] = { mat_hash };
				}
				asset_messages.job_queue.add_message(TextureDataJob{ tex_path, ERequestType::LoadFromFile, a_job.job_time});
			}
		}
		dep_tex_lock.unlock();
		mat_lock.unlock();
	}
}

void AssetJobThread::process_mesh_update(VAOLoadInfo& a_job)
{
	if (a_job.vao != 0)
	{
		GLuint vao = a_job.vao;
		GLsizei num_indices = 0;
		auto hash = a_job.hashed_path.hash;
		auto& mesh_map = asset_storage.mesh_map;
		{
			std::unique_lock<std::mutex> mesh_lock(asset_storage.mesh_mutex);
			if (mesh_map.contains(hash))
			{
				auto& mesh = mesh_map[hash];
				mesh->VAO = vao;
				mesh->vao_load_status = ELoadStatus::Loaded;
				mesh->contents->EBO = a_job.ebo;
				mesh->contents->VBOs = a_job.VBOs;
				num_indices = mesh->num_indicies;
				mesh->runtime_gen = ELoadStatus::StartedLoad;
			}
			else PRINTLN("Something went wrong after loading vao");
		}
		if (num_indices > 0 && vao > 0)
		{
			{
				auto& runtime_meshes = asset_storage.runtime_meshes;
				bool added_mesh = false;
				std::unique_lock<std::mutex> runtime_mesh_lock(asset_storage.runtime_mesh_mutex);
				for (size_t i = 0; i < runtime_meshes.size(); i++)
				{
					if (runtime_meshes[i].hash == hash)
					{
						break;
					}
					else if (runtime_meshes[i].hash > hash)
					{
						if (i + 1 < runtime_meshes.size())
						{
							runtime_meshes.emplace(runtime_meshes.begin() + i + 1,
								a_job.hashed_path.hash, num_indices, vao);
							added_mesh = true;
						}
						break;
					}
				}
				if (!added_mesh)
				{
					runtime_meshes.emplace_back(a_job.hashed_path.hash, num_indices, vao);
				}
			}
			std::unique_lock<std::mutex> mesh_lock(asset_storage.mesh_mutex);
			if (mesh_map.contains(hash))
			{
				auto& mesh = mesh_map[hash];
				mesh->runtime_gen = ELoadStatus::Loaded;
			}
			else PRINTLN("Something went wrong after loading runtime mesh");
		}
	}
	else
	{
		// deleted or failed to be uploaded... dunno what to do if it was failed...
	}
}

void AssetJobThread::process_tex_update(TextureGenInfo& a_job)
{
	// this is after a texgen data has been uploaded
	{
		auto& texs = asset_storage.tex_map;
		auto job_hash = a_job.hashed_path.hash;
		std::unique_lock<std::mutex> tex_lock(asset_storage.tex_mutex);
		if (texs.contains(job_hash))
		{
			auto& tex = texs[job_hash];
			if (a_job.modified_time < tex->modified_time)
				return;
			tex->texture_gen_status = ELoadStatus::Loaded;
			tex->modified_time = a_job.modified_time;
			tex->texture_id = a_job.texture_id;
		}
		else
		{
			PRINTLN("Texture in ram does not exist");
			return;
		}
		ValidateMatDependencies dep_job(a_job.hashed_path, EDependecyType::Texture, false, a_job.modified_time);
		asset_messages.job_queue.add_message(std::move(dep_job));
		// I think this is all...
	}
}

void AssetJobThread::process_program_update(ProgramLoadInfo& a_job)
{
	{
		auto& shaders = asset_storage.shader_map;
		auto job_hash = a_job.hashed_path.hash;
		std::unique_lock<std::mutex> shader_lock(asset_storage.shader_mutex);
		if (shaders.contains(job_hash))
		{
			auto& shader = shaders[job_hash];
			if (shader->modified_time > a_job.job_time)
			{
				return;
			}
			shader->gl_program = a_job.gl_program;
			shader->loading_status = ELoadStatus::ShaderLoadedProgram;
			shader->modified_time = a_job.job_time;
		}
		else
		{
			PRINTLN("shader in ram does not exist");
			return;
		}
		ValidateMatDependencies dep_job(a_job.hashed_path, EDependecyType::Shader, false, a_job.job_time);
		asset_messages.job_queue.add_message(std::move(dep_job));
	}
}

void AssetJobThread::process_dependency(ValidateMatDependencies& a_job) // this is super slow...
{
	if (a_job.dependency_check == EDependecyType::Texture && !a_job.unloaded)
	{
		auto& texs = asset_storage.tex_map;
		auto job_hash = a_job.hashed_path.hash;
		std::unique_lock<std::mutex> tex_lock(asset_storage.tex_mutex);
		if (texs.contains(job_hash)) // gotta check job time too I think
		{
			// this has to be done in basically every stage of handling this... including shader load and the acks
			auto& tex = texs[job_hash];
			if (tex->texture_gen_status == ELoadStatus::Loaded)
			{
				std::vector<uint64_t>& tex_deps = asset_storage.mat_tex_dependencies[tex->hash];
				auto& mats = asset_storage.mat_map;

				std::lock_guard<std::mutex> mat_guard(asset_storage.mat_mutex);
				std::lock_guard<std::mutex> tex_dep_guard(asset_storage.dep_tex_mutex);

				for (uint64_t tex_dep : tex_deps)
				{
					if (mats[tex_dep]->load_status == ELoadStatus::Loaded)
						continue;

					for (UniformData& uniform : mats[tex_dep]->uniforms) // insert needed info...
					{
						if (std::get_if<std::string>(&uniform.value))// it's a texture
						{
							uniform.texture_id = tex->texture_id;
							uniform.modified_time = a_job.job_time;
							break;
						}
					}
					uint8_t remaining_deps = 0;
					remaining_deps += (uint8_t)(mats[tex_dep]->gl_program != 0);
					for (TexDependency& dep : mats[tex_dep]->tex_deps)
					{
						if (dep.hash == job_hash)
						{
							dep.fulfilled = true;
							continue; // maybe there are multiple of same texture.. which would be bad, but eh
						}
						remaining_deps += (uint8_t)!dep.fulfilled;
					}

					if (remaining_deps == 0)
					{
						// generate runtime and mark loaded
						mats[tex_dep]->load_status = ELoadStatus::Loaded;
						uint64_t new_mat_hash = mats[tex_dep]->hashed_path.hash;
						std::lock_guard<std::mutex> mat_runtime_guard(asset_storage.runtime_mat_mutex);
						auto& runtime_mats = asset_storage.runtime_materials;
						bool inserted = false;
						size_t insertion_index = 0;
						for (size_t i = 0; i < runtime_mats.size(); i++)
						{
							uint64_t mat_hash = runtime_mats[i].material_hash;
							if (mat_hash == new_mat_hash)
							{
								inserted = true;
								break;
							}
							else if (new_mat_hash > mat_hash)
							{
								insertion_index = i + 1;
								break;
							}
						}
						if (!inserted)
						{
							//make the uniforms
							std::vector<RuntimeUniform> runtime_uniforms;
							for (UniformData& uniform : mats[new_mat_hash]->uniforms)
							{
								// it's weird, we'd really like to store the actual value in value... that is texture_id, but we
								if (auto string = std::get_if<std::string>(&uniform.value))// it's a texture
								{
									hashed_string_64 tex_hashed(string->c_str());
									runtime_uniforms.emplace_back(uniform.value, uniform.location.string, uniform.type, uniform.texture_id);
								}
								else
									runtime_uniforms.emplace_back(uniform.value, uniform.location.string, uniform.type, 0);
							}
							
							if (insertion_index < runtime_mats.size())
							{
								runtime_mats.emplace(runtime_mats.begin() + insertion_index,
									new_mat_hash, mats[new_mat_hash]->gl_program, runtime_uniforms);
							}
							else
							{
								runtime_mats.emplace_back(
									new_mat_hash, mats[new_mat_hash]->gl_program, runtime_uniforms);
							}
						}
					}
				}
			}
		}
	}
	else if (a_job.dependency_check == EDependecyType::Shader && !a_job.unloaded)
	{
		auto& shaders = asset_storage.shader_map;
		auto job_hash = a_job.hashed_path.hash;
		std::unique_lock<std::mutex> tex_lock(asset_storage.shader_mutex);
		if (shaders.contains(job_hash))
		{
			// this has to be done in basically every stage of handling this... including shader load and the acks
			auto& shader = shaders[job_hash];
			if (shader->loading_status == ELoadStatus::ShaderLoadedProgram)
			{
				std::vector<uint64_t>& shader_deps = asset_storage.mat_shader_dependencies[shader->hashed_path.hash];
				auto& mats = asset_storage.mat_map;

				std::lock_guard<std::mutex> mat_guard(asset_storage.mat_mutex);
				std::lock_guard<std::mutex> shader_dep_guard(asset_storage.dep_shader_mutex);

				for (uint64_t shader_dep : shader_deps)
				{
					if (mats[shader_dep]->load_status == ELoadStatus::Loaded)
						continue;

					uint8_t remaining_deps = 0;
					if (mats[shader_dep]->program_modified_time <= a_job.job_time)
					{
						mats[shader_dep]->gl_program = shader->gl_program;
					}
					remaining_deps = (int)mats[shader_dep]->gl_program == 0;
					for (TexDependency dep : mats[shader_dep]->tex_deps)
					{
						remaining_deps += (uint8_t)!dep.fulfilled;
					}
					if (remaining_deps == 0)
					{
						// generate runtime and mark loaded
						mats[shader_dep]->load_status = ELoadStatus::Loaded;
						uint64_t new_mat_hash = mats[shader_dep]->hashed_path.hash;
						std::lock_guard<std::mutex> mat_runtime_guard(asset_storage.runtime_mat_mutex);
						auto& runtime_mats = asset_storage.runtime_materials;
						bool inserted = false;
						size_t insertion_index = 0;
						for (size_t i = 0; i < runtime_mats.size(); i++)
						{
							uint64_t mat_hash = runtime_mats[i].material_hash;
							if (mat_hash == new_mat_hash)
							{
								inserted = true;
								break;
							}
							else if (new_mat_hash > mat_hash)
							{
								insertion_index = i + 1;
								break;
							}
						}
						if (!inserted)
						{
							//make the uniforms
							std::vector<RuntimeUniform> runtime_uniforms;
							for (UniformData& uniform : mats[new_mat_hash]->uniforms)
							{
								// it's weird, we'd really like to store the actual value in value... that is texture_id, but we
								if (std::get_if<std::string>(&uniform.value))// it's a texture
									runtime_uniforms.emplace_back(uniform.value, uniform.location.string, uniform.type, uniform.texture_id);
								else
									runtime_uniforms.emplace_back(uniform.value, uniform.location.string, uniform.type, 0);
							}
							if (insertion_index < runtime_mats.size())
							{
								runtime_mats.emplace(runtime_mats.begin() + insertion_index,
									new_mat_hash, mats[new_mat_hash]->gl_program, runtime_uniforms);
							}
							else
							{
								runtime_mats.emplace_back(
									new_mat_hash, mats[new_mat_hash]->gl_program, runtime_uniforms);
							}
						}
					}
				}
			}
		}
	}
}

void AssetJobThread::job_loop()
{
	while (true)
	{
		{
			std::unique_lock<std::mutex> thread_lock(*thread_sync_mutex);
			thread_cv->wait(thread_lock, [this]
				{
					return (this_exit || *all_exit) || !asset_messages.job_queue.is_empty();
				});
			if ((this_exit || *all_exit))
			{
				break;
			}
		}
		if (auto asset_job = asset_messages.job_queue.get_message())
		{
			auto& job = asset_job.value();
			bool invalid_job = true;
			if (auto mesh_job = std::get_if<MeshDataJob>(&job))
			{
				if (mesh_job->request_type == ERequestType::LoadFromFile) // add for delete later
				{
					process_load_mesh_job(*mesh_job);
					invalid_job = false;
				}
			}
			else if (auto tex_job = std::get_if<TextureDataJob>(&job))
			{
				if (tex_job->request_type == ERequestType::LoadFromFile)
				{
					process_tex_job(*tex_job);
					invalid_job = false;
				}
			}
				
			else if (auto gen_job = std::get_if<TextureGenInfo>(&job))
				process_tex_update(*gen_job);
			else if (auto vao_job = std::get_if<VAOLoadInfo>(&job))
			{
				process_mesh_update(*vao_job);
				invalid_job = false;
			}

			if (invalid_job)
				PRINTLN("INVALID JOB");
		}
	}
}

std::vector<RenderRequest> AssetManager::retrieve_render_requests(std::vector<PreRenderRequest>& a_pre_reqs)
{
	std::vector<RenderRequest> render_requests;
	render_requests.reserve(a_pre_reqs.size());
	hashed_string_64 invalid_hash;
	bool use_extra_sort = true;

	//if (a_request.size() * std::log(a_request.size())
	//	< (renderreqret_gen_storage.texuture_datas.size()* 8 + renderreqret_gen_storage.mesh_datas.size()*17 + a_request.size() * 11))
	if (true)
	{
		std::sort(a_pre_reqs.begin(), a_pre_reqs.end(), []
		(const PreRenderRequest& request_a, const PreRenderRequest& request_b)
			{
				return request_a.mesh_hash < request_b.mesh_hash;
			});
		std::lock_guard<std::mutex> mesh_guard(asset_storage.runtime_mesh_mutex);
		auto& meshes = asset_storage.runtime_meshes;
		size_t mesh_index = 0;
		size_t mesh_start_index = 0;
		bool mesh_found = false;
		for (auto& request : a_pre_reqs)
		{
			mesh_found = false;
			if (request.mesh_hash == invalid_hash.hash)
			{
				continue;
			}
			for (; mesh_index < meshes.size(); mesh_index++)
			{
				auto& mesh = meshes[mesh_index];
				if (request.mesh_hash == mesh.hash)
				{
					render_requests.emplace_back
					(request.model_matrix, request.mesh_hash, request.tex_hash, mesh.vao, mesh.num_indices, 0, 0);
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

		//std::sort(render_requests.begin(), render_requests.end(), []
		//(const RenderRequest& request_a, const RenderRequest& request_b)
		//	{
		//		return request_a.tex_hash < request_b.tex_hash;
		//	});

		//size_t tex_index = 0;
		//size_t tex_start_index = 0;
		//bool tex_found = false;
		//for (auto& request : render_requests)
		//{
		//	tex_found = false;
		//	if (request.tex_hash == invalid_hash)
		//	{
		//		request.material_id = 0;
		//		continue;
		//	}
		//	for (; tex_index < tex_datas.size(); tex_index++)
		//	{
		//		auto& data = tex_datas[tex_index];
		//		if (request.tex_hash == data.hash)
		//		{
		//			request.material_id = data.texture_id;
		//			tex_start_index = tex_index;
		//			tex_found = true;
		//			break;
		//		}

		//	}
		//	if (!tex_found)
		//	{
		//		tex_index = tex_start_index;
		//	}
		//}
		//PRINTLN("used extra sort");
	}
	//else
	//{
	//	std::sort(a_request.begin(), a_request.end(), []
	//	(const PreRenderRequest& request_a, const PreRenderRequest& request_b)
	//		{
	//			if (request_a.mesh_hash == request_b.mesh_hash)
	//				return request_a.tex_hash < request_b.tex_hash;
	//			return request_a.mesh_hash < request_b.mesh_hash;

	//		});
	//	std::lock_guard<std::mutex> mesh_guard(asset_storage.runtime_mesh_mutex);
	//	auto& meshes = asset_storage.runtime_meshes;

	//	std::lock_guard<std::mutex> mesh_guard(renderreqret_gen_storage.mesh_mutex);
	//	std::lock_guard<std::mutex> tex_guard(renderreqret_gen_storage.texture_mutex);
	//	auto& tex_datas = renderreqret_gen_storage.texuture_datas;
	//	auto& mesh_datas = renderreqret_gen_storage.mesh_datas;
	//	size_t mesh_index = 0;
	//	size_t mesh_start_index = 0;
	//	bool mesh_found = false;
	//	size_t tex_index = 0;
	//	size_t tex_start_index = 0;
	//	GLuint tex_id = 0;
	//	GLuint vao = 0;
	//	GLsizei size = 0;
	//	bool tex_found = false;
	//	uint64_t prev_mesh_hash = invalid_hash.hash;
	//	bool requested_mesh = false;
	//	bool requested_tex = false;
	//	for (auto& request : a_request)
	//	{
	//		tex_id = 0;
	//		vao = 0;
	//		size = 0;
	//		mesh_found = false;
	//		if (request.mesh_hash == invalid_hash.hash)
	//			continue;
	//		if (prev_mesh_hash != request.mesh_hash)
	//		{
	//			tex_index = 0;
	//			tex_start_index = 0;
	//		}

	//		for (; mesh_index < mesh_datas.size(); mesh_index++) // should check mesh/tex if it's loaded, if not, add it as load job.
	//		{
	//			MeshData& mesh_data = mesh_datas[mesh_index];
	//			if (request.mesh_hash == mesh_data.hash)
	//			{
	//				vao = mesh_data.VAO;
	//				size = mesh_data.num_indicies;

	//				mesh_start_index = mesh_index;
	//				prev_mesh_hash = mesh_data.hash;
	//				mesh_found = true;
	//				tex_index = 0;
	//				tex_found = false;
	//				if (request.tex_hash != invalid_hash.hash)
	//				{
	//					for (; tex_index < tex_datas.size(); tex_index++)
	//					{
	//						TextureData& tex_data = tex_datas[tex_index];
	//						if (request.tex_hash == tex_data.hash)
	//						{
	//							tex_id = tex_data.texture_id;

	//							tex_start_index = tex_index;
	//							tex_found = true;
	//							break;
	//						}
	//					}
	//				}
	//				if (!tex_found)
	//					tex_index = tex_start_index;

	//				break;
	//			}
	//		}
	//		if (!mesh_found)
	//		{
	//			mesh_index = mesh_start_index;
	//		}

	//		else
	//		{
	//			render_requests.emplace_back
	//			(request.model_matrix, request.mesh_hash, request.tex_hash, vao, size, 0, tex_id);
	//		}
	//	}
	//}

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
