#include <iostream>

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

#include <entt/entt.hpp>

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


int main(void)
{
	GLFWwindow* window;

	///* Initialize the library */
	if (!glfwInit())
		return -1;

	std::shared_ptr<GLFWContext> glfw_context = std::make_shared<GLFWContext>(1280, 960, "Icicl engine", true, true);
	glfw_context->deactivate();
	std::shared_ptr<ImGuiManager> imgui_manager = std::make_shared<ImGuiManager>(glfw_context);
	glfw_context->activate();

	///* Initialize ImGUI */
	//ImGuiContext* imgui_context; // this needs to be shared
	//imgui_context = ImGui::CreateContext();
	//ImGuiIO& io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;		  // Enable Multi-Viewport / Platform Windows

	//ImGui::StyleColorsDark();
	//ImGuiStyle& style = ImGui::GetStyle();
	//ImGui_ImplGlfw_InitForOpenGL(window, true);
	//ImGui_ImplOpenGL3_Init("#version 460");

	//float test = 5.0f;
	//bool demo = true;
	//if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	//{
	//	style.WindowRounding = 0.0f;
	//	style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	//}

	// load the sahder

	/* Loop until the user closes the window */

	//glfwSwapInterval(0);
	//glEnable(GL_DEPTH_TEST);

	//glDisable(GL_CULL_FACE);

	std::shared_ptr<Scene> scene = std::make_shared<Scene>();
	{
		entt::hashed_string string("./assets/obj/triobjmonkey.obj");
		std::weak_ptr<SceneObject> withtChild = scene->new_scene_object("with Child", true);
		std::weak_ptr<SceneObject> withoutChild = scene->new_scene_object("without Child", true);
		if (auto shared = withoutChild.lock())
		{
			shared->add_component_data<WorldPositionComponentData>(WorldPositionComponent{ glm::vec3(0.f,0.f,0.f) });
			entt::hashed_string path("./assets/obj/sizanne.obj");
			shared->add_component_data<MeshComponentData>(MeshComponent{ 0, path.data(), path});
		}
		std::weak_ptr<SceneObject> wChild = scene->new_scene_object("without Child", false);
		std::weak_ptr<SceneObject> ChildwChild = scene->new_scene_object("Child with child", false);
		std::weak_ptr<SceneObject> childofchild = scene->new_scene_object("Child of Child", false);
		if (auto parent = withtChild.lock())
		{
			parent->add_component_data<WorldPositionComponentData>(WorldPositionComponent{glm::vec3(0,1,2)});
			parent->replace_component_data(WorldPositionComponent{ glm::vec3(1,2,3) });
			parent->add_or_replace_component_data<WorldPositionComponentData>(WorldPositionComponent{ glm::vec3(2,3,4) });
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
					shared->add_component_data<WorldPositionComponentData>(WorldPositionComponent{ glm::vec3(-1.f,0.f,0.f) });
					shared->add_component_data<RenderableComponentData>(RenderableComponent{ 2,3 });
					shared->remove_component_data<NameComponentData>();
					entt::hashed_string string("./assets/obj/triobjmonkey.obj");
					shared->add_component_data<MeshComponentData>(MeshComponent{0, string.data(), string});
				}
			}
		}
	}

	std::shared_ptr<UIManager> ui_mananger  = std::make_shared<UIManager>();
	ui_mananger->set_scene(scene);

	ObjParser obj_parser;
	//MeshData mesh = obj_parser.load_mesh_from_filepath("./assets/obj/triobjmonkey.obj");//triobjmonkey
	VAOLoader vao_loader;
	//vao_loader.load_vao(mesh);
	std::shared_ptr<ShaderProgram> shader_program = std::make_shared<ShaderProgram>("./assets/shaders/vertex/vert.glsl", "./assets/shaders/fragment/frag.glsl");
	Renderer renderer;
	renderer.temp_set_shader(shader_program);

	std::shared_ptr<MeshDataGenStorage> storage = 
		std::make_shared<MeshDataGenStorage>(2);
	std::shared_ptr<EngineContext> engine_context = std::make_shared<EngineContext>(storage);
	
	//RenderThread render_thread(engine_context, *shader_program, glfw_context);
	GameThread game_thread(engine_context, scene);
	//EngineThread engine_thread(engine_context, imgui_manager, ui_mananger);

	std::vector<std::unique_ptr<std::thread>> threads;
	

	//threads.push_back(std::make_unique<std::thread>(enginer_thread));
	//threads.push_back(std::make_unique<std::thread>(renderer_thread));
	//threads.push_back(std::make_unique<std::thread>(gamer_thread));

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
	while (engine_context->run())
	{
		////////////////////////////////////////////////////
		// RENDERING -- needs reordering
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
		glfw_context->activate();
		glfw_context->clear();

		auto& render_requests = engine_context->render_requests[std::size_t(!engine_context->write_pos)];
		for (size_t i = 0; i < render_requests.size(); i++)
		{
			//PRINTLN("vao: {}, size: {}, entity: {}", render_requests[i].vao, render_requests[i].indices_size, render_requests[i].shader_program);
			if (render_requests[i].vao != 0)
			{
				renderer.temp_render(render_requests[i]);
			}
		}

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
			else
			{
				// I don't know what to do...
			}
			//vao_loader.load_vao(vao_request);
			// load in vao..
			// then I have to change the meshdata entry to notify that I've loaded the vao.
		}
		
		frames++;
		timer.stop();
		total_time += timer.get_time_ms();
		timer.start();
		if (total_time > 1000)
		{
			title = "Icicl engine - FPS: " + std::to_string(frames/(1000.0 / total_time));
			//title = "Icicl engine - frame time: " + std::to_string(total_time);
			glfwSetWindowTitle(window, title.c_str());
			total_time = 0;
			frames = 0;
		}
		
		{
			std::unique_lock<std::mutex> lock(engine_context->mutex);
			engine_context->cv_frame_coordinator.wait(lock, [engine_context] 
				{ return (!engine_context->game_thread || !engine_context->run()); });
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
			ui_mananger->draw_object_hierarchy();
			ui_mananger->draw_object_properties();
			imgui_manager->render();
		}
		engine_context->cv_threads.notify_all();
		//////////////////////////////////////////
		//ImGui ends
		glfwPollEvents();

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
