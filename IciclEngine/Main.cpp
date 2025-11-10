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
#include "Texture.h"
#include "Material.h"
#include "GameObject.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#ifndef ASSIMP_LOAD_FLAGS
#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices)
#endif


int main(void)
{

	//TEMP
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::mat4 view;
	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)640 / (float)480, 0.1f, 100.0f);

	glm::mat4 model = glm::mat4(1.0f);
	//END TEMP

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
	const aiScene* pScene = importer.ReadFile("./assets/fbx/monkey.fbx", ASSIMP_LOAD_FLAGS);
	bool ret = false;

	// TODO: Create a Meshloader class
	// Load mesh and add the VBO and EBO to the Mesh class ... Figure out how to deal with multi materials


	if (pScene)
	{
		if (pScene->HasMeshes())
		{
			std::println("Has Mesh");
			for (size_t i = 0; i < pScene->mNumMeshes; i++)
			{
				std::string stdstr = pScene->mMeshes[i]->GetTextureCoordsName(0)->C_Str();
				std::println("Mesh {} has Positons: {}, UVs: {}, Colors: {}, Has Normals: {}", i, pScene->mMeshes[i]->HasPositions(), pScene->mMeshes[i]->HasTextureCoords(0), pScene->mMeshes[i]->HasVertexColors(0), pScene->mMeshes[i]->HasNormals());
				std::println("tangets stuff {}", pScene->mMeshes[i]->HasTangentsAndBitangents());
				std::println("has texture coords names: {},texture coords name {}, num of textures: {}", pScene->mMeshes[i]->HasTextureCoordsName(0), stdstr , pScene->mMaterials[0]->GetTextureCount(aiTextureType_DIFFUSE));
				//for (size_t j = 0; j < pScene->mMeshes[i]->mNumVertices; j++)
				//{
				//	std::print("({}, {}, {}) ", pScene->mMeshes[i]->mVertices[i].x, pScene->mMeshes[i]->mVertices[i].y, pScene->mMeshes[i]->mVertices[i].z);
				//}
				//std::cout << std::endl;
				//std::println("positions: {}", pScene->mMeshes[i]->mVertices);
				aiString path;
				if (pScene->mMaterials[0]->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS || true) {
					std::string texturePath(path.C_Str()); // This is the texture file path relative to the model
					std::println("texute path: {}", texturePath);
					// Load your texture from this path in your engine
				}
				
			}
		}
	}


	Shader* triangleShader = new Shader("./assets/shaders/vertex/vert.glsl", "./assets/shaders/fragment/frag.glsl");
	Shader* otherShader = new Shader("./assets/shaders/vertex/vert2.glsl", "./assets/shaders/fragment/frag.glsl");
	Mesh* myMesh = new Mesh("./assets/fbx/monkey.fbx");
	Mesh* girl = new Mesh("./assets/fbx/girl.fbx");
	Texture* texture = new Texture("./assets/textures/wall.jpg");
	Material* material = new Material(triangleShader, std::vector<Texture*>{ texture });

	ForwardRenderer* forwardRenderer = new ForwardRenderer();
	forwardRenderer->AddRenderPass(*triangleShader);
	forwardRenderer->AddRenderPass(*otherShader);
	EngineContext* context = new EngineContext((Renderer*)forwardRenderer, scene); // very simple tests gives 5% faster to sort
	//EngineContext* context = new EngineContext(renderer, scene);


	auto start = std::chrono::high_resolution_clock::now();
	Mesh* mesh;
	{
		std::vector<float> vertices = {
		-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f
		};
		std::vector<unsigned int> indices = { 0, 1, 2 };
		std::vector<unsigned int> offsets = { 3, 3, 2 };

		//Triangle* triangle = new Triangle();
		mesh = new Mesh(std::move(indices), std::move(vertices), std::move(offsets)); // ~20% faster
		//mesh = new Mesh(indices, vertices, offsets);
		//std::cout << "Size of vertices = " << vertices.size() << std::endl;
	}
	auto stop = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

	Renderable* renderable = new Renderable(myMesh, triangleShader);
	Renderable* renderable2 = new Renderable(girl, triangleShader);
	std::vector<GameObject*> gameObjects;
	gameObjects.push_back(new GameObject(glm::vec3(1.0f, 0.0, -2.0f), renderable));
	gameObjects.push_back(new GameObject(glm::vec3(-1.0f, 0.0, -3.0f), renderable));
	renderer->Init();
	scene->AddRenderable(renderable);
	scene->AddRenderable(renderable2);


	/* Loop until the user closes the window */
	int itters = 0;
	glfwSwapInterval(0);
	glEnable(GL_DEPTH_TEST);
	//glDisable(GL_CULL_FACE);
	while (!glfwWindowShouldClose(window))
	{
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
		glUseProgram(triangleShader->GetShaderProgram());
		triangleShader->SetMat4fv(proj, "projection");
		triangleShader->SetMat4fv(view, "view");
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, -3.0f));
		model = glm::rotate(model, (float)glfwGetTime() * glm::radians(100.0f), glm::vec3(1.0f, 0.3f, 0.5f));
		triangleShader->SetMat4fv(model, "model");
		glUseProgram(0);
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
