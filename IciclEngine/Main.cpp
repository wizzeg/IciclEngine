#include <iostream>

#include <imgui-docking/imgui.h>
#include <imgui-docking/imgui_impl_glfw.h>
#include <imgui-docking/imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <KHR/khrplatform.h>
#include <GLFW/glfw3.h>

#include "Shader.h"
#include "Triangle.h"
#include "Mesh.h"

#include <chrono>
#include <print>

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


	Shader* triangleShader = new Shader("./assets/shaders/vertex/vert.glsl", "./assets/shaders/fragment/frag.glsl");
	Mesh* mesh;
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

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT);

		glClearColor(0.1f, 0.3f, 0.2f, 1.0f);


#pragma region Render
		//Render stuff here
		triangleShader->Use();
		mesh->Render();
		triangleShader->Stop();
		//Render stuff end here
#pragma endregion

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
