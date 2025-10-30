#include <iostream>

#include <imgui-docking/imgui.h>
#include <imgui-docking/imgui_impl_glfw.h>
#include <imgui-docking/imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <KHR/khrplatform.h>
#include <GLFW/glfw3.h>

#include <chrono>
#include <print>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Scene.h"
#include "Renderer.h"
#include "Shader.h"
#include "Triangle.h"
#include "Mesh.h"
#include "EngineContext.h"
#include "ForwardRenderer.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


int main(void)
{

	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;
	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
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

	Scene* scene = new Scene();
	Renderer* renderer = new Renderer();

	Assimp::Importer importer;

	Shader* triangleShader = new Shader("./assets/shaders/vertex/vert.glsl", "./assets/shaders/fragment/frag.glsl");
	Shader* otherShader = new Shader("./assets/shaders/vertex/vert2.glsl", "./assets/shaders/fragment/frag.glsl");
	Mesh* mesh;
	ForwardRenderer* forwardRenderer = new ForwardRenderer();
	forwardRenderer->AddRenderPass(*triangleShader);
	forwardRenderer->AddRenderPass(*otherShader);
	EngineContext* context = new EngineContext((Renderer*)forwardRenderer, scene); // very simple tests gives 5% faster to sort
	//EngineContext* context = new EngineContext(renderer, scene);
	auto start = std::chrono::high_resolution_clock::now();

	{
		std::vector<float> vertices = {
		-0.5f, -0.5f, 0.0f, 1.0, 0.0, 0.0, 0.0,
		0.5f, -0.5f, 0.0f, 0.0, 1.0, 0.0, 0.0,
		0.0f, 0.5f, 0.0f, 0.0, 0.0, 1.0, 0.0
		};
		std::vector<unsigned int> indices = { 0, 1, 2 };
		std::vector<unsigned int> offsets = { 3, 3, 1 };

		//Triangle* triangle = new Triangle();
		mesh = new Mesh(std::move(indices), std::move(vertices), std::move(offsets)); // ~20% faster
		//mesh = new Mesh(indices, vertices, offsets);
		//std::cout << "Size of vertices = " << vertices.size() << std::endl;
	}
	auto stop = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
	std::cout << "Time to do a copy: " << duration.count() << std::endl;
	std::println("C++23 println test... Time to move: {}", duration.count());

	Renderable* renderable = new Renderable(mesh, triangleShader);
	Renderable* renderable2 = new Renderable(mesh, otherShader);
	Renderable* renderable3 = new Renderable(mesh, triangleShader);
	Renderable* renderable4 = new Renderable(mesh, otherShader);
	Renderable* renderable5 = new Renderable(mesh, triangleShader);
	Renderable* renderable6 = new Renderable(mesh, otherShader);
	Renderable* renderable7 = new Renderable(mesh, triangleShader);
	Renderable* renderable8 = new Renderable(mesh, otherShader);
	scene->AddRenderable(renderable);
	scene->AddRenderable(renderable2);
	scene->AddRenderable(renderable3);
	scene->AddRenderable(renderable4);
	scene->AddRenderable(renderable5);
	scene->AddRenderable(renderable6);
	scene->AddRenderable(renderable7);
	//scene->RemoveRenderable(renderable);
	scene->AddRenderable(renderable8);
	renderer->Init();

	/* Loop until the user closes the window */
	int itters = 0;
	glfwSwapInterval(0);
	while (!glfwWindowShouldClose(window))
	{
		
		/* Render here */
		//glClear(GL_COLOR_BUFFER_BIT);
		//glClearColor(0.1f, 0.3f, 0.2f, 1.0f);

		//Enginecontext
		if (itters == 0) start = std::chrono::high_resolution_clock::now();
#pragma region Render
		//Render stuff here
		
		context->DrawScene();
		//triangleShader->Use();
		//mesh->Render();
		//triangleShader->Stop();
		//Render stuff end here
#pragma endregion

		itters++;
		if (itters >= 2500)
		{
			stop = std::chrono::high_resolution_clock::now();
			auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
			//std::cout << "average render time: " << (duration2.count()/ ((long double)itters)) << "millseconds" << std::endl;
			std::println("average render time : {:.6f}ms", (duration2.count() / ((long double)itters * 1000.0f)));
			itters = 0;
		}

#pragma region ImGui
		// ImGUI draw
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("test window");
		ImGui::Text("test text");
		
		ImGui::SliderFloat("test slider", &test, 0.0, 10.0);
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
			GLFWwindow* backup_current_context = glfwGetCurrentContext();

			// Update and Render additional Platform Windows
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();

			// restore current GL context.
			glfwMakeContextCurrent(backup_current_context);
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
