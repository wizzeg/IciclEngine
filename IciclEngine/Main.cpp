#include <iostream>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui-docking/imgui.h>
#include <imgui-docking/imgui_impl_glfw.h>
#include <imgui-docking/imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <KHR/khrplatform.h>
#include <GLFW/glfw3.h>

#include <chrono>
//
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>

//#include <assimp/Importer.hpp>
//#include <assimp/scene.h>
//#include <assimp/postprocess.h>
//#include <engine/utilities/entt_modified.h>
//#include <entt/entt.hpp>

#include <engine/editor/scene.h>
#include <engine/editor/scene_object.h>
#include <engine/utilities/macros.h>
#include <engine/game/components.h>

//#ifndef ASSIMP_LOAD_FLAGS
//#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices)
//#endif
#include <engine/ui/ui_manager.h>
//#include <engine/resources/obj_parser.h>
#include <engine/renderer/vao_loader.h>
//#include <engine/renderer/shader_program.h>
#include <engine/renderer/renderer.h>
#include <engine/core/game_thread.h>

#include <thread>
//#include <engine/resources/data_storage.h>
#include <engine/renderer/glfw_context.h>
#include <engine/ui/imgui_manager.h>
#include <engine/utilities/utilities.h>
#include <engine/game/input_manager.h>
#include <engine/game/camera.h>

#include <engine/utilities/memory_checking.h>
#include <engine/editor/field_serialization_entires.h>
#include <engine/editor/component_entries.h>
#include <engine/resources/asset_manager.h>
#include <engine/renderer/shader_loader.h>
#include <engine/editor/systems_registry.h>
#include <engine/editor/systems_entries.h>

#include <algorithm>


int main(void)
{
	GLFWwindow* window;
	PRINTLN("default hash = {}", (hashed_string_64("")).hash);
	///* Initialize the library */
	if (!glfwInit())
		return -1;

	std::shared_ptr<GLFWContext> glfw_context = std::make_shared<GLFWContext>(1920, 1080, "Icicl engine", true, true, true);
	glfw_context->deactivate();
	std::shared_ptr<ImGuiManager> imgui_manager = std::make_shared<ImGuiManager>(glfw_context);
	glfw_context->activate();
	glfw_context->create_framebuffer("editor_frame_buffer", 2560, 1440, Output);
	glfw_context->bind_framebuffer("editor_frame_buffer"); // Need to do this every frame really, when I'm changing framebuffers
	//glEnable(GL_CULL_FACE);
	glfw_context->unbind_framebuffer();
	glfw_context->create_framebuffer("main_camera_buffer", 2560, 1440, Output);

	glfw_context->unbind_framebuffer();
	glfw_context->create_framebuffer("editor_camera_gbuffer", 2560, 1440, GBuffer);


	glfw_context->unbind_framebuffer();
	glfw_context->create_framebuffer("main_camera_gbuffer", 2560, 1440, GBuffer);

	glfw_context->create_framebuffer("shadow_map_array", 2048, 2048, EFramebufferType::ShadowMapArray);
	FrameBuffer* shadow_maps = glfw_context->get_framebuffer("shadow_map_array");

	//imgui_manager->setup_default_docking_layout();

	//InputManager input_manager(glfw_context->get_window());

	///////////
	// Making scene and adding test scene_objects
	std::shared_ptr<Scene> scene = std::make_shared<Scene>();
	//scene->load("./assets/scenes/scene.scn", true);
	std::shared_ptr<UIManager> ui_mananger  = std::make_shared<UIManager>();
	ui_mananger->set_scene(scene);

	//ObjParser obj_parser;
	VAOLoader vao_loader;
	std::shared_ptr<ShaderProgram> shader_program = std::make_shared<ShaderProgram>("./assets/shaders/vertex/vert.glsl", "./assets/shaders/fragment/frag.glsl");
	Renderer renderer(glfw_context);
	renderer.initialize();
	renderer.temp_set_shader(shader_program);
	renderer.set_shadow_maps(shadow_maps);
	shader_program->save("./assets/shaders/test_shader.shdr");

	ShaderProgram shader_load;
	shader_load.load("./assets/shaders/test_shader.shdr");
	shader_load.save("./assets/shaders/test_shader2.shdr");
	//std::shared_ptr<MeshDataGenStorage> storage = std::make_shared<MeshDataGenStorage>(2);
	std::shared_ptr<EngineContext> engine_context = std::make_shared<EngineContext>(scene/*storage*/);
	
	//RenderThread render_thread(engine_context, *shader_program, glfw_context);
	GameThread game_thread(engine_context, scene);
	//EngineThread engine_thread(engine_context, imgui_manager, ui_mananger);

	bool game_playing = false;


	//std::thread enginer_thread(&EngineThread::execute, &engine_thread);
	//std::thread renderer_thread(&RenderThread::execute, &render_thread);
	std::thread gamer_thread(&GameThread::execute, &game_thread);

	HighResolutionTimer timer;
	timer.start();
	double total_time = 0;
	size_t frames = 0;
	std::string title = "";

	window = glfw_context->get_window();
	ImGuiIO* io = imgui_manager->get_io();

	InputManager& input_manager = InputManager::instance();
	bool captured_prev_frame = false;
	ImVec2 mouse_pos;

	HighResolutionTimer timer2;
	HighResolutionTimer render_call_timer;
	double render_load_time = 0;
	double render_call_time = 0;
	double render_thread_time = 0;
	double ui_manager_time = 0;
	int framies = 0;

	//AssetManager asset_manager;
	//MeshDataJob job("./assets/obj/triobjmonkey.obj", ERequestType::LoadFromFile);
	////AssetJob asset_job = std::move(job);
	//asset_manager.add_asset_job(std::move(job));

	uint64_t wait_time = 0;
	glfwSetWindowFocusCallback(window, [](GLFWwindow* window, int focused)
		{
			if (!focused)
			{
				InputManager::instance().unlock_mouse();
			}
		});

	while (engine_context->run())
	{
		timer2.start();
		if (wait_time > 45)
		{
			if (!game_playing)
			{
				{
					
					std::lock_guard<std::mutex> guard(engine_context->mutex);
					//scene->start_runtime(); // deal with making a runtime copy later -------- runtime thing works at least, entities are created
					// for now I need to be able to see changes to entities -> handle signaling
					game_playing = true;
					//engine_context->game_playing = true;
					std::srand(static_cast<unsigned>(std::time(nullptr)));
					hashed_string_64 monkey_mesh = hashed_string_64("./assets/obj/triobjmonkey.obj");
					hashed_string_64 tex = hashed_string_64("./assets/textures/awesomeface.png");

					std::vector<hashed_string_64> meshes = {  "./assets/obj/cube.obj" }; //"./assets/obj/plane.obj","./assets/obj/triobjmonkey.obj" , "./assets/obj/sizanne.obj", "./assets/obj/robot.obj","./assets/obj/robot2.obj","./assets/obj/robot3.obj"
					std::vector<hashed_string_64> texes = { "./assets/textures/awesomeface.png", "./assets/textures/wall.jpg", "./assets/textures/container.jpg" };
					std::vector<hashed_string_64> mats = { "./assets/shaders/white.mat" }; // , "./assets/shaders/test2.mat" "./assets/shaders/test.mat", "./assets/shaders/testcopy.mat"
					
					auto processor = scene->new_scene_object("Single thread processor", true);
					processor.lock()->add_component<SingleProcessorComponent>();

					auto landscape = scene->new_scene_object("landscape", true);
					auto ls = landscape.lock();
					ls->add_component(TransformDynamicComponent{ glm::vec3(0, -200, 0), glm::vec3(512, 1, 512) });
					ls->add_component(LandscapeComponent{ hashed_string_64("./assets/textures/landscape/heightmap.png"), 2, 0, 0 });
					ls->add_component(RenderComponent{hashed_string_64("./assets/obj/landscape/landscape256.obj"), hashed_string_64("./assets/shaders/landscape/landscape.mat"), false, false, true});

					{
						auto obj = scene->new_scene_object("parent ", true);
						auto scene_object = obj.lock();
						auto obj2 = scene->new_scene_object("child ", true);
						auto scene_object2 = obj2.lock();
						auto obj3 = scene->new_scene_object("child 2", true);
						auto scene_object3 = obj3.lock();
						auto obj4 = scene->new_scene_object("child 4", true);
						auto scene_object4 = obj4.lock();
						auto obj5 = scene->new_scene_object("child 5", true);
						auto scene_object5 = obj5.lock();
						auto obj6 = scene->new_scene_object("child 6", true);
						auto scene_object6 = obj6.lock();
						scene->parent_scene_object(obj, obj2);
						scene->parent_scene_object(obj, obj3);
						//scene_object->paren(scene_object2);
						scene->parent_scene_object(obj2, obj3);
						//scene->parent_scene_object(obj3, obj2);
						scene->parent_scene_object(obj, obj4);
						scene->parent_scene_object(obj, obj5);
						scene->parent_scene_object(obj2, obj6);
					}

					/*{
						for (auto mesh : meshes)
						{
							auto obj = scene->new_scene_object(std::string("mesh loader for: ") + mesh.string, true);
							auto scene_object = obj.lock();
							scene_object->add_component(MeshComponent{false, mesh});
						}
						{
							auto obj = scene->new_scene_object(std::string("mesh loader for: ") + "./assets/obj/triobjmonkey.obj", true);
							auto scene_object = obj.lock();
							scene_object->add_component(MeshComponent{ false, "./assets/obj/triobjmonkey.obj" });
						}
						
						for (auto mat : mats)
						{
							auto obj = scene->new_scene_object(std::string("mat loader for: ") + mat.string, true);
							auto scene_object = obj.lock();
							scene_object->add_component(MaterialComponent{ mat, true, true, true });
						}

						std::vector<hashed_string_64> bleh({ "./assets/shaders/red.mat", "./assets/shaders/green.mat", "./assets/shaders/blue.mat", "./assets/shaders/white.mat" });
						for (auto mat : bleh)
						{
							auto obj = scene->new_scene_object(std::string("mat loader for: ") + mat.string, true);
							auto scene_object = obj.lock();
							scene_object->add_component(MaterialComponent{ mat, true, true, true });
						}
					}*/


					for (size_t i = 0; i < 2500; i++)
					{
						float x = -200.0f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / 400.0f));
						float y = -200.0f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / 400.0f));; // Or random if you want variety
						float z = -200.0f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / 400.0f));; // Same here
						auto obj = scene->new_scene_object(std::string("model: ") + std::to_string(i), true);
						auto scene_object = obj.lock();
						size_t mesh_idx = std::rand() % meshes.size();
						size_t tex_idx = std::rand() % texes.size();
						size_t mat_idx = std::rand() % mats.size();
						scene_object->add_component(TransformDynamicComponent{ glm::vec3(x, y, z)}); //(glm::normalize(glm::abs(glm::vec3(x, y, z))) + glm::vec3(0.5f))
						//scene_object->add_component(MeshComponent{ false, meshes[mesh_idx]});
						//scene_object->add_component(MaterialComponent{mats[mat_idx], false, false, true});
						scene_object->add_component(RenderComponent{ meshes[mesh_idx], mats[mat_idx], true, true, true });
						//scene_object->add_component(TextureComponent{ false, texes[tex_idx]});
						scene_object->add_component(SpawnPositionComponent{ glm::vec3(x, y, z) });
						scene_object->add_component(BoundingBoxComponent{ glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.f), false, 2, 32 });

						float mass = 1.f;
						float inverse_mass = 1.f;
						float random = std::rand() % 50;
						glm::vec3 linear_velocity = normalize(glm::vec3(x, y, z)) * -0.f;
						glm::vec3 rotation_velocity = normalize(glm::vec3(x, y, z)) * -0.2f;
						if (glm::length(glm::vec3(x, y, z)) < 0.f)
						{
							mass = 100.f;	
							inverse_mass = 0.01f;
							linear_velocity = glm::vec3(0.f);
							rotation_velocity = glm::vec3(0.f);
						}
						scene_object->add_component(RigidBodyComponent{ glm::vec3(x, y, z), glm::quat(1.0f, 0.f, 0.f, 0.f), linear_velocity, rotation_velocity, mass, inverse_mass, glm::mat3(0), 1.0f, 0.4f});
						if (auto rb = scene_object->try_get_component<RigidBodyComponent>())
						{
							rb->set_dynamic_layer(1);
						}
					}

					for (size_t i = 0; i < 20; i++)
					{
						float x = -50.0f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / 100.0f));
						float y = -50.0f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / 100.0f));; // Or random if you want variety
						float z = -50.0f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / 100.0f));; // Same here
						auto obj = scene->new_scene_object(std::string("light: ") + std::to_string(i), true);
						auto scene_object = obj.lock();
						int col = (std::rand() % 3);
						glm::vec3 color(0);
						std::string mat;
						switch (col)
						{
						case 1:
							color = glm::vec3(1,0,0);
							mat = "./assets/shaders/red.mat";
							break;
						case 2:
							color = glm::vec3(0, 1, 0);
							mat = "./assets/shaders/green.mat";
							break;
						case 0:
							color = glm::vec3(0, 0, 1);
							mat = "./assets/shaders/blue.mat";
							break;
						default:
							color = glm::vec3(1);
							mat = "./assets/shaders/white.mat";
							break;
						}
						//glm::vec3 color((std::rand() % 1000)* 0.001f, (std::rand() % 1000) * 0.001f, (std::rand() % 1000) * 0.001f);
						float intensity = 0.75f + (std::rand() % 250) * 0.001;
						glm::vec3 attenuation = glm::vec3(0.75f, 0.1f, 0.01f);
						hashed_string_64 name(mat.c_str());
						scene_object->add_component(TransformDynamicComponent{ glm::vec3(x, y, z), glm::vec3(0.5f)});
						scene_object->add_component(PointLightComponent{color, attenuation, intensity, false});
						//scene_object->add_component(MeshComponent{ false, "./assets/obj/triobjmonkey.obj" });
						//scene_object->add_component(MaterialComponent{ name, false, false, true});
						scene_object->add_component(RenderComponent{ "./assets/obj/triobjmonkey.obj", name, false, true, true });
					}

				}
				//scene->start_runtime(); // deal with making a runtime copy later -------- runtime thing works at least, entities are created
				//// for now I need to be able to see changes to entities -> handle signaling
				//game_playing = true;
				//engine_context->game_playing = true;
			}
		}
		else
		{
			wait_time++;
		}
		////////////////////////////////////////////////////
		// RENDERING 
		glfw_context->swap_buffers();
		glfw_context->deactivate();
		//PRINTLN("render thread going to sleep: {}", runs++);

		if (!engine_context->run())
		{
			engine_context->cv_frame_coordinator.notify_all();
			engine_context->cv_threads.notify_all();
			break;
		}

		if (glfw_context->window_should_close()) engine_context->kill_all = true;
		
		glfw_context->activate(); /// just sets this window as the active context... hmm...
		///////////////////////
		//Render the render requests
		// Here this would be repeated for every camera ... Yes, I suppose so... Do they bind the frame buffer themselves, perhaps?
		// Lets just start with a hardcoded editor camera.
		auto& render_requests = engine_context->render_requests[std::size_t(!engine_context->write_pos)];
		auto& render_context = engine_context->render_contexts[std::size_t(!engine_context->write_pos)];

		renderer.set_camera_position(engine_context->editor_camera.get_camera_position());
		renderer.set_proj_view_matrix(engine_context->editor_camera.get_proj_matrix(), engine_context->editor_camera.get_view_matrix());
		glm::vec3 camera_pos = engine_context->editor_camera.get_camera_position();

		DefferedBuffer deffered_buffer;
		deffered_buffer.gbuffer = glfw_context->get_framebuffer("editor_camera_gbuffer");
		deffered_buffer.output = glfw_context->get_framebuffer("editor_frame_buffer");
		deffered_buffer.shadow_maps = glfw_context->get_framebuffer("shadow_map_array");
		render_call_timer.start();
		renderer.deffered_render(render_context, deffered_buffer);
		render_call_timer.stop();
		render_call_time += render_call_timer.get_time_ms();

		// otherrender target stuff... but for now this is basically only for main camera...
		for (size_t i = 0; i < engine_context->cameras_render[std::size_t(!engine_context->write_pos)].size(); i++)
		{
				glfw_context->clear();
				renderer.set_camera_position(engine_context->cameras_render[std::size_t(!engine_context->write_pos)][i].position);
				renderer.set_proj_view_matrix(engine_context->cameras_render[std::size_t(!engine_context->write_pos)][i].proj_matrix,
				engine_context->cameras_render[std::size_t(!engine_context->write_pos)][i].view_matrix);

				deffered_buffer.gbuffer = glfw_context->get_framebuffer("main_camera_gbuffer");
				deffered_buffer.output = glfw_context->get_framebuffer("main_camera_buffer");
				deffered_buffer.shadow_maps = glfw_context->get_framebuffer("shadow_map_array");
				renderer.deffered_render(render_context, deffered_buffer);
		}

	
		//////////////////////////////////////////////////////////////////////
		// New graphics load system
		render_call_timer.start();
		if (auto vao_req = engine_context->asset_manager->get_vao_request())
		{
			MeshData& mesh_data = vao_req.value().mesh_data;
			if (vao_loader.load_vao(mesh_data))
			{
				PRINTLN("Render sending the new form of vao upate request message");
				AssetJob load_job = std::move(
					VAOLoadInfo{ 
						mesh_data.contents->hashed_path, mesh_data.VAO, mesh_data.contents->EBO, 0, mesh_data.contents->VBOs,   });
				engine_context->asset_manager->add_asset_job(load_job);
			}
		}
		if (auto shader_req = engine_context->asset_manager->get_program_request())
		{
			auto& shader_data = shader_req.value().shader_data;
			GLint shader_program = ShaderLoader::compile_shader(shader_data);
			if (shader_program != 0)
			{
				PRINTLN("Render sending the new form of vao upate request message");
				AssetJob load_job = std::move(
					ProgramLoadInfo{
						shader_data.hashed_path, shader_program, ELoadStatus::ShaderLoadedProgram, 0 });
				engine_context->asset_manager->add_asset_job(load_job);
			}
		}
		if (auto tex_req = engine_context->asset_manager->get_gen_request())
		{
			TextureData& tex_data = tex_req.value().texture_data;
			PRINTLN("Got texture gen request");
			TextureGenInfo ret_val = engine_context->texture_loader->generate_texture(tex_data);
			if (ret_val.texture_gen_status == ELoadStatus::Loaded)
			{
				AssetJob load_job = std::move(TextureGenInfo{ ret_val.hashed_path, ret_val.texture_id, ret_val.texture_gen_status });
				engine_context->asset_manager->add_asset_job(load_job);
			}
		}
		render_call_timer.stop();
		render_load_time += render_call_timer.get_time_ms();
		
		timer2.stop();
		render_thread_time += timer2.get_time_ms();

		{
			glfw_context->unbind_framebuffer();
			std::unique_lock<std::mutex> lock(engine_context->mutex);
			engine_context->cv_frame_coordinator.wait(lock, [engine_context] 
				{ return (!engine_context->game_thread || !engine_context->run()); });
		}
		glfwPollEvents();
		frames++;
		timer.stop();
		engine_context->delta_time = std::min(timer.get_time_s(), 0.25);
		timer.start();
		total_time += engine_context->delta_time; // this should be on game thread too: stop at start of frame -> record dt -> immedietly into start
		if (total_time > 1)
		{
			title = "Icicl engine - FPS: " + std::to_string(frames / (1.0 / total_time));
			//title = "Icicl engine - frame time: " + std::to_string(total_time);
			glfwSetWindowTitle(window, title.c_str());
			total_time = 0;
			frames = 0;
		}
		
		
		////////////////////////////////////////////////////////////////
		///// Rendering ends
		///// may as well do rendering now on main thread...
		///////////////////////////////////////////////////////////////
		//// ImGui starts
		{
			std::lock_guard<std::mutex> guard(engine_context->mutex);
			timer2.start();
			engine_context->swap_render_requests();
			engine_context->render_thread = true;
			engine_context->game_thread = true;
			auto load_scene = engine_context->systems_context->get_system_storage().consume_object<LoadSceneCommand>("LoadSceneCommand");
			if (load_scene && load_scene->load)
			{
				scene->load(load_scene->path);
			}
			engine_context->systems_context->get_system_storage().perform_erase();
			
			// do all ui drawing.
			imgui_manager->new_frame();

			imgui_manager->make_dockspace();
			ImGui::Begin("editor_frame_buffer");
			ImVec2 available_content = ImGui::GetContentRegionAvail() - ImVec2(5, 5);
			ImVec2 image_size;
			FrameBuffer* frame_buffer = glfw_context->get_framebuffer("editor_frame_buffer");
			//FrameBuffer* frame_buffer = deffered_buffer.output;
			if (frame_buffer)
			{
				float fb_height = (float)frame_buffer->get_height();
				float fb_width = (float)frame_buffer->get_width();
				
				float fb_aspect = (float)fb_height / (float)fb_width;
				float avail_aspect = (float)available_content.x / (float)available_content.y;

				if ((available_content.x / fb_width) < (available_content.y / fb_height))
				{
					image_size = ImVec2(available_content.x, available_content.x * fb_aspect);
				}
				else
				{
					image_size = ImVec2(available_content.y / fb_aspect, available_content.y);
				}

			}
			else
			{
				image_size = ImVec2(720, 480);
			}
			ImGui::Image(frame_buffer->get_color_texture(), image_size, ImVec2(0, 1), ImVec2(1, 0));
			if (engine_context->input_manager.is_key_held(EKey::RightMouseButton) && ImGui::IsItemHovered()) // all of this should be put into camera
			{
				ImGui::SetMouseCursor(ImGuiMouseCursor_None);
				if (!captured_prev_frame) /// not sure where I should put this bool... cache in the camera perhaps? Yes probably
				{
					mouse_pos = ImGui::GetMousePos();
					captured_prev_frame = true;
				}
				ImVec2 mouse_delta = ImGui::GetMousePos() - mouse_pos;
				engine_context->editor_camera.set_camera_movable(true);
				engine_context->editor_camera.move(glm::vec2(mouse_delta.x, -mouse_delta.y));
				SetCursorPos((int)mouse_pos.x, (int)mouse_pos.y); // windows function... not pretty, but works
				
				// I'll also need something like this when the play button is pressed, to reset the mouse position all the time...use ~ to lose capture?
				// so the mouse capture has to be some kind of singleton perhaps
			}
			else
			{
				engine_context->editor_camera.set_camera_movable(false);
				captured_prev_frame = false;
			}
			ImGui::End();

			ImGui::Begin("main_camera_buffer");
			available_content = ImGui::GetContentRegionAvail() - ImVec2(5, 5);
			frame_buffer = glfw_context->get_framebuffer("main_camera_buffer");
			if (frame_buffer)
			{
				float fb_height = (float)frame_buffer->get_height();
				float fb_width = (float)frame_buffer->get_width();

				float fb_aspect = (float)fb_height / (float)fb_width;
				float avail_aspect = (float)available_content.x / (float)available_content.y;

				if ((available_content.x / fb_width) < (available_content.y / fb_height))
				{
					image_size = ImVec2(available_content.x, available_content.x * fb_aspect);
				}
				else
				{
					image_size = ImVec2(available_content.y / fb_aspect, available_content.y);
				}
			}
			else
			{
				image_size = ImVec2(720, 480);
			}
			ImGui::Image(frame_buffer->get_color_texture(), image_size, ImVec2(0, 1), ImVec2(1, 0));
			if (engine_context->input_manager.is_key_pressed(EKey::LeftMouseButton) && ImGui::IsItemHovered()) // all of this should be put into camera
			{
				ImVec2 item_min = ImGui::GetItemRectMin();
				ImVec2 item_max = ImGui::GetItemRectMax();
				ImVec2 center = ImVec2((item_min.x + item_max.x) * 0.5f, (item_min.y + item_max.y) * 0.5f);
				engine_context->input_manager.lock_mouse(center.x, center.y);
			}
			if (engine_context->input_manager.is_mouse_locked())
			{
				ImGui::SetMouseCursor(ImGuiMouseCursor_None); // don't know if this will work in real build...
				if (engine_context->input_manager.is_key_pressed(EKey::Tilde))
					engine_context->input_manager.unlock_mouse();
			}

			ImGui::End();

			ImGui::Begin("editor_camera_gbuffer-albedo_spec_texture");
			available_content = ImGui::GetContentRegionAvail() - ImVec2(5, 5);
			frame_buffer = glfw_context->get_framebuffer("editor_camera_gbuffer");
			if (frame_buffer)
			{
				float fb_height = (float)frame_buffer->get_height();
				float fb_width = (float)frame_buffer->get_width();

				float fb_aspect = (float)fb_height / (float)fb_width;
				float avail_aspect = (float)available_content.x / (float)available_content.y;

				if ((available_content.x / fb_width) < (available_content.y / fb_height))
				{
					image_size = ImVec2(available_content.x, available_content.x * fb_aspect);
				}
				else
				{
					image_size = ImVec2(available_content.y / fb_aspect, available_content.y);
				}
			}
			else
			{
				image_size = ImVec2(720, 480);
			}
			image_size.x = std::max((float)image_size.x, 1.0f);
			image_size.y = std::max((float)image_size.y, 1.0f);
			GLuint texture = frame_buffer->get_albedo_spec_texture();
			if (texture == 0)
			{
				PRINTLN("texture 0");
			}
			ImGui::Image(texture , image_size, ImVec2(0, 1), ImVec2(1, 0));
			ImGui::End();

			ImGui::Begin("editor_camera_gbuffer-normal_buffer");
			available_content = ImGui::GetContentRegionAvail() - ImVec2(5, 5);
			frame_buffer = glfw_context->get_framebuffer("editor_camera_gbuffer");
			if (frame_buffer)
			{
				float fb_height = (float)frame_buffer->get_height();
				float fb_width = (float)frame_buffer->get_width();

				float fb_aspect = (float)fb_height / (float)fb_width;
				float avail_aspect = (float)available_content.x / (float)available_content.y;

				if ((available_content.x / fb_width) < (available_content.y / fb_height))
				{
					image_size = ImVec2(available_content.x, available_content.x * fb_aspect);
				}
				else
				{
					image_size = ImVec2(available_content.y / fb_aspect, available_content.y);
				}
			}
			else
			{
				image_size = ImVec2(720, 480);
			}
			image_size.x = std::max((float)image_size.x, 1.0f);
			image_size.y = std::max((float)image_size.y, 1.0f);
			texture = frame_buffer->get_normal_texture();
			if (texture == 0)
			{
				PRINTLN("texture 0");
			}
			ImGui::Image(texture, image_size, ImVec2(0, 1), ImVec2(1, 0));
			ImGui::End();

			ImGui::Begin("editor_camera_gbuffer-position_buffer");
			available_content = ImGui::GetContentRegionAvail() - ImVec2(5, 5);
			frame_buffer = glfw_context->get_framebuffer("editor_camera_gbuffer");
			if (frame_buffer)
			{
				float fb_height = (float)frame_buffer->get_height();
				float fb_width = (float)frame_buffer->get_width();

				float fb_aspect = (float)fb_height / (float)fb_width;
				float avail_aspect = (float)available_content.x / (float)available_content.y;

				if ((available_content.x / fb_width) < (available_content.y / fb_height))
				{
					image_size = ImVec2(available_content.x, available_content.x * fb_aspect);
				}
				else
				{
					image_size = ImVec2(available_content.y / fb_aspect, available_content.y);
				}
			}
			else
			{
				image_size = ImVec2(720, 480);
			}
			image_size.x = std::max((float)image_size.x, 1.0f);
			image_size.y = std::max((float)image_size.y, 1.0f);
			texture = frame_buffer->get_position_texture();
			if (texture == 0)
			{
				PRINTLN("texture 0");
			}
			ImGui::Image(texture, image_size, ImVec2(0, 1), ImVec2(1, 0));
			ImGui::End();

			//ImGui::SetNextWindowSize(ImVec2(1280, 960));

			bool demo = true;
			//ImGui::ShowDemoWindow(&demo);
			ui_mananger->draw_systems();
			ui_mananger->draw_object_hierarchy();
			ui_mananger->draw_object_properties();
			//ui_mananger->draw_selected_icon(engine_context->editor_camera.get_view_matrix(),
			//	engine_context->editor_camera.get_proj_matrix());

			//ui_mananger->RenderContentBrowser();
			ui_mananger->render_play_stop(engine_context.get());
			ui_mananger->draw_menubar();
			//ui_mananger->RenderTopMenuBar();
			//ui_mananger->RenderToolbar();

			//input_manager.update_input();
			engine_context->update_input();

			imgui_manager->render();
			timer2.stop();
			ui_manager_time += timer2.get_time_ms();
		}
		if (framies > 500 && false)
		{
			PRINTLN("render draw calls dispatch timer: {}", render_call_time / (double)framies);
			PRINTLN("render thread loading timer: {}", render_load_time / (double)framies);
			PRINTLN("render thread timer: {}", render_thread_time / (double)framies);
			PRINTLN("ui manager frametime: {}", ui_manager_time / (double)framies);
			render_load_time = 0;
			render_call_time = 0;
			render_thread_time = 0;
			ui_manager_time = 0;
			framies = 0;
		}
		else framies++;
		engine_context->cv_threads.notify_all();
		//////////////////////////////////////////
		//ImGui ends
		
		

	}

	//if (enginer_thread.joinable()) enginer_thread.join();
	//if (renderer_thread.joinable()) renderer_thread.join();
	if (gamer_thread.joinable()) gamer_thread.join();

	//for (size_t i = 0; i < threads.size(); i++)
	//{
	//	if (threads[i].get()->joinable())
	//	{
	//		threads[i].get()->join();
	//	}
	//}
	/// No more opengl or imgui

	//RenderContext glfw_context;
	//engine_context.
	
	while (false && !glfwWindowShouldClose(window))
	{
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//glClearColor(0.1f, 0.3f, 0.2f, 1.0f);



		//////////////////////// systems start here for now

		//if (game_playing)
		//{
		//	auto& registry = scene.get()->get_registry();

		//	for (auto [entity, name, worldpos] : registry.view<EntityComponent, TransformDynamicComponent>().each())
		//	{
		//		worldpos.position.x += 0.0005f;
		//	}

		//	for (auto [entity, renderable, world_position] : registry.view<RenderableComponent, TransformDynamicComponent>().each())
		//	{
		//		renderer.temp_render(mesh, world_position);
		//	}
		//}
		//else
		//{
		//	auto scene_objects = scene.get()->get_scene_objects();
		//	for (size_t i = 0; i < scene_objects.size(); i++)
		//	{
		//		if (RenderableComponent* renderable; scene_objects[i].get()->get_component(renderable))
		//		{
		//			if (TransformDynamicComponent* world_pos; scene_objects[i].get()->get_component(world_pos))
		//			{
		//				renderer.temp_render(mesh, *world_pos);
		//			}
		//		}
		//	}
		//}


		//for (auto [entity, name] : registry.view<EntityComponent>().each())
		//{
		//	std::cout << (name.name.c_str()) << std::endl;
		//}

		/////////////////////// systems end here for now



		///* Render here */
		//Enginecontext
#pragma region Render
		//Render stuff here
		

		//Render stuff end here
#pragma endregion

#pragma region ImGui
		//// ImGUI draw
		//ImGui_ImplOpenGL3_NewFrame();
		//ImGui_ImplGlfw_NewFrame();
		//ImGui::NewFrame();

		//ImGui::SetNextWindowSize(ImVec2(500, 400));
		//ui_mananger.draw_object_hierarchy();
		//ui_mananger.draw_object_properties();

		////ImGui::Begin("UI Manager Window");
		////bool focused = ImGui::IsWindowFocused();
		////ImGui::Text("Focused: %s", focused ? "Yes" : "No");
		////ImGui::End();

		//if (demo)
		//	ImGui::ShowDemoWindow(&demo);
		//if (test >= 9.999999)
		//	demo = true;
		//ImGui::Render();
		//ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); // this needs the context ready

		//// obviously this needs the context ready too
		//if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		//{
		//	// Capture the current GL context
		//	GLFWwindow* contextBackup = glfwGetCurrentContext();

		//	// Update and Render additional Platform Windows
		//	ImGui::UpdatePlatformWindows();
		//	ImGui::RenderPlatformWindowsDefault();

		//	// restore current GL context.
		//	glfwMakeContextCurrent(contextBackup);
		//}

		// ImGUI stop draw
#pragma endregion
		
		

		///* Swap front and back buffers */
		//glfwSwapBuffers(window);

		///* Poll for and process events */
		//glfwPollEvents();


	}

	/* Cleanup */
	window = glfwGetCurrentContext(); // I might need this, shouldn't hurt. ... well now I certainly dont need it though?? what??
	glfwMakeContextCurrent(window);
	glfwDestroyWindow(window);
	imgui_manager->destroy();
	glfwTerminate();
	return 0;
}
