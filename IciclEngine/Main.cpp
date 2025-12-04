#include <iostream>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui-docking/imgui.h>
#include <imgui-docking/imgui_impl_glfw.h>
#include <imgui-docking/imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <KHR/khrplatform.h>
#include <GLFW/glfw3.h>

#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <engine/utilities/entt_modified.h>
//#include <entt/entt.hpp>

#include <engine/editor/scene.h>
#include <engine/editor/scene_object.h>
#include <engine/utilities/macros.h>
#include <engine/game/components.h>

#ifndef ASSIMP_LOAD_FLAGS
#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices)
#endif
#include <engine/ui/ui_manager.h>
#include <engine/resources/obj_parser.h>
#include <engine/renderer/vao_loader.h>
#include <engine/renderer/shader_program.h>
#include <engine/renderer/renderer.h>
#include "worker_thread.h"

#include <thread>
#include <engine/resources/data_storage.h>
#include "glfw_context.h"
#include "imgui_manager.h"
#include <engine/utilities/utilities.h>
#include "input_manager.h"
#include "camera.h"

#include <engine/utilities/memory_checking.h>


int main(void)
{
	GLFWwindow* window;

	///* Initialize the library */
	if (!glfwInit())
		return -1;

	std::shared_ptr<GLFWContext> glfw_context = std::make_shared<GLFWContext>(450, 1, "Icicl engine", true, true);
	glfw_context->deactivate();
	std::shared_ptr<ImGuiManager> imgui_manager = std::make_shared<ImGuiManager>(glfw_context);
	glfw_context->activate();
	glfw_context->create_framebuffer("editor_frame_buffer", 720, 480);
	glfw_context->bind_framebuffer("editor_frame_buffer"); // Need to do this every frame really, when I'm changing framebuffers
	glfw_context->unbind_framebuffer();
	glfw_context->create_framebuffer("main_camera_buffer", 720, 480);
	glfw_context->bind_framebuffer("editor_frame_buffer");
	//InputManager input_manager(glfw_context->get_window());

	///////////
	// Making scene and adding test scene_objects
	std::shared_ptr<Scene> scene = std::make_shared<Scene>();
	{
		auto camera = scene->new_scene_object("Camera test", true);
		if (auto c = camera.lock())
		{
			hashed_string_64 buffer_name("main_camera_buffer");
			c->add_component_data<WorldPositionComponentData, WorldPositionComponent>(WorldPositionComponent{ glm::vec3(0.0f, 0.0f, 10.0f) });
			c->add_component_data<CameraComponentData, CameraComponent>
				(CameraComponent{});
		}

		auto camera2 = scene->new_scene_object("Camera test2", true);
		if (auto c = camera2.lock())
		{
			hashed_string_64 buffer_name("main_camera_buffer");
			c->add_component_data<WorldPositionComponentData, WorldPositionComponent>(WorldPositionComponent{ glm::vec3(5.0f, 5.0f, 10.0f) });
			c->add_component_data<CameraComponentData, CameraComponent>
				(CameraComponent{});
		}
		hashed_string_64 string("./assets/obj/triobjmonkey.obj");
		std::weak_ptr<SceneObject> withtChild = scene->new_scene_object("with Child", true);
		std::weak_ptr<SceneObject> withoutChild = scene->new_scene_object("without Child", true);
		if (auto shared = withoutChild.lock())
		{
			shared->add_component_data<WorldPositionComponentData>(WorldPositionComponent{ glm::vec3(0.f,0.f,0.f) });
			hashed_string_64 path("./assets/obj/sizanne.obj");
			shared->add_component_data<MeshComponentData>(MeshComponent{ 0, path});
		}
		std::weak_ptr<SceneObject> wChild = scene->new_scene_object("without Child", false);
		std::weak_ptr<SceneObject> ChildwChild = scene->new_scene_object("Child with child", false);
		std::weak_ptr<SceneObject> childofchild = scene->new_scene_object("Child of Child", false);
		if (auto parent = withtChild.lock())
		{
			parent->add_component_data<WorldPositionComponentData>(WorldPositionComponent{glm::vec3(0,1,2)});
			parent->replace_component_data(WorldPositionComponent{ glm::vec3(1,2,3) });
			parent->add_or_replace_component_data<WorldPositionComponentData>(WorldPositionComponent{ glm::vec3(2,3,4) });
			parent->add_component_data<MeshComponentData>(MeshComponent{0, string});
			WorldPositionComponent* test;
			if (parent->get_component(test))
			{
				PRINTLN("x value: {}", test->position.x);
				test->position.x = 10;
				PRINTLN("x value: {}", test->position.x);
			}
			else {
				PRINTLN("failes");
			}
			parent->add_child(wChild);
			parent->add_child(ChildwChild);
			if (auto shared = ChildwChild.lock())
			{
				shared->add_child(childofchild);
				if (auto shared = childofchild.lock())
				{
					
					if (MaterialComponent* mat_test; shared->get_component<MaterialComponent>(mat_test))
					{
						PRINTLN("GOT MAT TEST SOMEHOW");
					}
					
					shared->add_component_data<WorldPositionComponentData>(WorldPositionComponent{ glm::vec3(-1.f,0.f,0.f) });
					shared->add_component_data<RenderableComponentData>(RenderableComponent{ 2,3 });
					shared->remove_component_data<NameComponentData>();
					hashed_string_64 string("./assets/obj/triobjmonkey.obj");
					shared->add_component_data<MeshComponentData>(MeshComponent{0, string });
				}
			}
		}
	}


	std::shared_ptr<UIManager> ui_mananger  = std::make_shared<UIManager>();
	ui_mananger->set_scene(scene);

	ObjParser obj_parser;
	VAOLoader vao_loader;
	std::shared_ptr<ShaderProgram> shader_program = std::make_shared<ShaderProgram>("./assets/shaders/vertex/vert.glsl", "./assets/shaders/fragment/frag.glsl");
	Renderer renderer;
	renderer.temp_set_shader(shader_program);

	std::shared_ptr<MeshDataGenStorage> storage = std::make_shared<MeshDataGenStorage>(2);
	std::shared_ptr<EngineContext> engine_context = std::make_shared<EngineContext>(storage);
	
	//RenderThread render_thread(engine_context, *shader_program, glfw_context);
	GameThread game_thread(engine_context, scene);
	//EngineThread engine_thread(engine_context, imgui_manager, ui_mananger);

	bool game_playing = false;
	if (!game_playing)
	{
		{
			std::lock_guard<std::mutex> guard(engine_context->mutex);
			scene->to_runtime(); // deal with making a runtime copy later -------- runtime thing works at least, entities are created
			// for now I need to be able to see changes to entities -> handle signaling
			game_playing = true;
			engine_context.get()->game_playing = true;
		}
	}

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

	InputManager& input_manager = InputManager::get();
	bool captured_prev_frame = false;
	ImVec2 mouse_pos;
	while (engine_context->run())
	{
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

		// do for each camera.
		renderer.rotation += 0.07f * engine_context->delta_time * 0.01;
		glfw_context->bind_framebuffer("editor_frame_buffer");
		glfw_context->clear();
		renderer.set_proj_view_matrix(engine_context->editor_camera.get_proj_matrix(), engine_context->editor_camera.get_view_matrix());
		for (size_t i = 0; i < render_requests.size(); i++)
		{
			if (render_requests[i].vao != 0)
			{
				renderer.temp_render(render_requests[i]);
			}
		}
		glfw_context->bind_framebuffer("main_camera_buffer");
		glfw_context->clear();
		// render for each ingame camera
		for (size_t i = 0; i < engine_context->cameras_render[std::size_t(!engine_context->write_pos)].size(); i++)
		{
			if (glfw_context->bind_framebuffer(engine_context->cameras_render[std::size_t(!engine_context->write_pos)][i].frame_buffer_hashed.string))
			{
				glfw_context->clear();
				renderer.set_proj_view_matrix(engine_context->cameras_render[std::size_t(!engine_context->write_pos)][i].proj_matrix,
					engine_context->cameras_render[std::size_t(!engine_context->write_pos)][i].view_matrix);
				for (size_t i = 0; i < render_requests.size(); i++)
				{
					if (render_requests[i].vao != 0)
					{
						renderer.temp_render(render_requests[i]);
					}
				}
			}
		}

		/////////////////////////////////////////
		// Loading a VAO request
		if (auto vao_request = engine_context->storage->get_vao_request())
		{
			MeshData& mesh_data = vao_request.value().mesh_data;
			PRINTLN("Render thread got vao request");
			if (vao_loader.load_vao(mesh_data))
			{
				PRINTLN("Render sending vao upate request message");
				VAOLoadInfo load_info{mesh_data.path_hashed, mesh_data.VAO_loaded, mesh_data.VAO, mesh_data.VBOs, mesh_data.EBO};
				engine_context->storage->update_vao_info(load_info);
			}
			//else // I don't know what to do...
		}
		


		
		{
			glfw_context->unbind_framebuffer();
			std::unique_lock<std::mutex> lock(engine_context->mutex);
			engine_context->cv_frame_coordinator.wait(lock, [engine_context] 
				{ return (!engine_context->game_thread || !engine_context->run()); });
		}
		glfwPollEvents();
		frames++;
		timer.stop();
		engine_context->delta_time = timer.get_time_ms();
		timer.start();
		total_time += engine_context->delta_time; // this should be on game thread too: stop at start of frame -> record dt -> immedietly into start
		if (total_time > 1000)
		{
			title = "Icicl engine - FPS: " + std::to_string(frames / (1000.0 / total_time));
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
			engine_context->swap_render_requests();
			engine_context->render_thread = true;
			engine_context->game_thread = true;

			// do all ui drawing.
			imgui_manager->new_frame();
			ImGui::Begin("editor_frame_buffer");
			ImGui::Image(glfw_context->get_framebuffer_texture("editor_frame_buffer"), ImVec2(720, 480), ImVec2(0, 1), ImVec2(1, 0));
			if (engine_context->input_manager.is_key_held(EKey::RightMouseButton) && ImGui::IsItemHovered()) // all of this should be put into camera
			{
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

			ImGui::Image(glfw_context->get_framebuffer_texture("main_camera_buffer"), ImVec2(720, 480), ImVec2(0, 1), ImVec2(1, 0));

			ImGui::End();

			//ImGui::SetNextWindowSize(ImVec2(1280, 960));

			bool demo = true;
			ImGui::ShowDemoWindow(&demo);

			ui_mananger->draw_object_hierarchy();
			ui_mananger->draw_object_properties();

			//input_manager.update_input();
			input_manager.update_input();
			if (input_manager.is_key_held(EKey::A))
			{
				//PRINTLN("HELD A for: {}s", input_manager.key_held_duration(EKey::A));
			}

			imgui_manager->render();
		}
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

		//	for (auto [entity, name, worldpos] : registry.view<NameComponent, WorldPositionComponent>().each())
		//	{
		//		worldpos.position.x += 0.0005f;
		//	}

		//	for (auto [entity, renderable, world_position] : registry.view<RenderableComponent, WorldPositionComponent>().each())
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
		//			if (WorldPositionComponent* world_pos; scene_objects[i].get()->get_component(world_pos))
		//			{
		//				renderer.temp_render(mesh, *world_pos);
		//			}
		//		}
		//	}
		//}


		//for (auto [entity, name] : registry.view<NameComponent>().each())
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
	imgui_manager->destroy();
	glfwTerminate();
	return 0;
}
