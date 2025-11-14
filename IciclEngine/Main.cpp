#include <iostream>

#include <imgui-docking/imgui.h>
#include <imgui-docking/imgui_impl_glfw.h>
#include <imgui-docking/imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <KHR/khrplatform.h>
#include <GLFW/glfw3.h>

#include <chrono>
//#include <print>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <entt/entt.hpp>

#include "scene.h"
#include "entity.h"
#include "macros.h"

#ifndef ASSIMP_LOAD_FLAGS
#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices)
#endif


int main(void)
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;
	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(1280, 960, "Hello World", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}
	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize Glad" << std::endl;
		return -1;
	}
	/* Initialize ImGUI */
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;		  // Enable Multi-Viewport / Platform Windows

	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	float test = 5.0f;
	bool demo = true;
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// load the sahder

	/* Loop until the user closes the window */
	int itters = 0;
	glfwSwapInterval(0);
	glEnable(GL_DEPTH_TEST);
	//glDisable(GL_CULL_FACE);

	////////////////////////////////////////
	// Idea...
	// Create a wrapper around entity, which can add itself to world (register) if it wants to, and do all the entity stuff
	// have pre-system and post-system, perhaps a mid-system phase with IciclBehavior which is normal OOP attachable classes
	// The IciclBehavior then as OnEarlyUpdate, and OnUpdate, and perhaps some more, "hooks" to run after/before entt systems
	// this way you can make a game without any ECS, or one fully in ECS, or a hybrid.
	///////////////////////////////////////

	std::shared_ptr<Scene> scene = std::make_shared<Scene>();
	std::weak_ptr<SceneObject> tempobj =  scene.get()->new_scene_object("new scene object");
	if (auto shared = tempobj.lock())
	{
		shared->add_component_data<NameComponentData>(NameComponent{"testing with name"});
		NameComponent test;
		shared->get_component<NameComponent, NameComponentData>(test);
	}
	
	tempobj = scene.get()->new_scene_object("different name");
	if (auto shared = tempobj.lock())
	{
		shared->add_component_data<NameComponentData>(NameComponent{ "different name" });
		NameComponent test;
		shared->get_component<NameComponent, NameComponentData>(test);
		shared->add_component_data<NameComponentData>(NameComponent{ "different name2" });
		shared->replace_component_data<NameComponentData>(NameComponent{ "different name replaced" });
		shared->add_or_replace_component_data<NameComponentData>(NameComponent{ "different name added or replaced" });
		shared->add_component_data<WorldPositionComponentData>(WorldPositionComponent{ glm::vec3(1.6f, 1.f, 1.f) });
	}

	bool game_playing = false;
	std::shared_ptr<Scene> runtimeScene = std::make_shared<Scene>();
	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (!game_playing)
		{

			// do runtime scene here
			///////////////////////////////////
			// What should it do?
			// we are to convert scene_objects into Entity, and IF in editor, then also create scene objects...
			// should be placed in a new scene
			// So, FIRST we make Entity, then we make the scene_object ... no I need two versions, one for runtime and one for not runtime.
			game_playing = true;
		}
		///* Render here */

		//Enginecontext
#pragma region Render
		//Render stuff here
		

		//Render stuff end here
#pragma endregion

#pragma region ImGui
		// ImGUI draw
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::SetNextWindowSize(ImVec2(500, 400));
		ImGui::Begin("GameObjects list");
		
		//const std::vector<std::shared_ptr<SceneObject>> scene_objects = scene.get()->get_scene_objects();
		//for (size_t i = 0; i < scene_objects.size(); i++)
		//{
		//	scene_objects[i].get()->draw_components();
		//	//if (auto shared = scene_objects[i].get()->get_entity().lock())
		//	//{
		//	//	auto handle = shared->get_handle();
		//	//	auto val = static_cast<uint32_t>(handle.entity());
		//	//	PRINTLN("handle: {}", val);
		//	//}
		//	//
		//}
		scene->draw_imgui();

		ImGui::End();
		if (demo)
			ImGui::ShowDemoWindow(&demo);
		if (test >= 9.999999)
			demo = true;
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			// Capture the current GL context
			GLFWwindow* contextBackup = glfwGetCurrentContext();

			// Update and Render additional Platform Windows
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();

			// restore current GL context.
			glfwMakeContextCurrent(contextBackup);
		}

		// ImGUI stop draw
#pragma endregion
		
		

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();


	}

	/* Cleanup */
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}
