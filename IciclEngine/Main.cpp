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
#include "scene_object.h"
#include "entity.h"
#include "macros.h"

#ifndef ASSIMP_LOAD_FLAGS
#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices)
#endif
#include "ui_manager.h"


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
	{

		std::weak_ptr<SceneObject> withtChild = scene.get()->new_scene_object("with Child", true);
		std::weak_ptr<SceneObject> withoutChild = scene.get()->new_scene_object("without Child", true);
		if (auto shared = withoutChild.lock())
		{
			shared->add_component_data<WorldPositionComponentData>(WorldPositionComponent{ glm::vec3(3.f,2.f,1.f) });
		}
		std::weak_ptr<SceneObject> wChild = scene.get()->new_scene_object("without Child", false);
		std::weak_ptr<SceneObject> ChildwChild = scene.get()->new_scene_object("Child with child", false);
		std::weak_ptr<SceneObject> childofchild = scene.get()->new_scene_object("Child of Child", false);
		if (auto parent = withtChild.lock())
		{
			parent->add_child(wChild);
			parent->add_child(ChildwChild);
			if (auto shared = ChildwChild.lock())
			{
				shared->add_child(childofchild);
				if (auto shared = childofchild.lock())
				{
					shared->add_component_data<WorldPositionComponentData>(WorldPositionComponent{ glm::vec3(3.f,5.f,1.f) });
					shared->add_component_data<RenderableComponentData>(RenderableComponent{ 2,3 });
					shared->remove_component_data<NameComponentData>();
				}
			}
		}
	}

	/*std::weak_ptr<SceneObject> tempobj =  scene.get()->new_scene_object("new scene object");*/
	//if (auto shared = tempobj.lock())
	//{
	//	shared->add_component_data<NameComponentData>(NameComponent{"testing with name"});
	//	NameComponent test;
	//	shared->get_component<NameComponent, NameComponentData>(test);
	//}
	//
	//tempobj = scene.get()->new_scene_object("different name");
	//if (auto shared = tempobj.lock())
	//{
	//	shared->add_component_data<NameComponentData>(NameComponent{ "different name" });
	//	NameComponent test;
	//	shared->get_component<NameComponent, NameComponentData>(test);
	//	shared->add_component_data<NameComponentData>(NameComponent{ "different name2" });
	//	shared->replace_component_data<NameComponentData>(NameComponent{ "different name replaced" });
	//	shared->add_or_replace_component_data<NameComponentData>(NameComponent{ "different name added or replaced" });
	//	shared->add_component_data<WorldPositionComponentData>(WorldPositionComponent{ glm::vec3(1.6f, 1.f, 1.f) });
	//	shared->add_component_data<RenderableComponentData>(RenderableComponent{3, 2});
	//}

	//auto tempobj2 = scene.get()->new_scene_object("child object");
	//if (auto shared = tempobj.lock())
	//{
	//	shared->add_child(tempobj2);
	//}
	UIManager ui_mananger = UIManager();
	ui_mananger.set_scene(scene);

	bool game_playing = false;
	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.1f, 0.3f, 0.2f, 1.0f);

		if (!game_playing)
		{
			// do runtime scene here
			///////////////////////////////////
			// What should it do?
			// we are to convert scene_objects into Entity, and IF in editor, then also create scene objects...
			// should be placed in a new scene
			// So, FIRST we make Entity, then we make the scene_object ... no I need two versions, one for runtime and one for not runtime.



			// Idea, when an entity is going to add a component, then also the scene must be notified, this is only way to
			// I guess same could be done for removal and adding entities aswell.
			// I guess I'll always make some kind of "ecb" that takes those requests, and will then handle all it needs to do.

			//scene.get()->to_runtime(); // deal with making a runtime copy later -------- runtime thing works at least, entities are created
			// for now I need to be able to see changes to entities -> handle signaling
			game_playing = true;
		}

		//////////////////////// systems start here for now


		auto& registry = scene.get()->get_registry();

		for (auto [entity, name, worldpos] : registry.view<NameComponent, WorldPositionComponent>().each())
		{
			//registry.remove<WorldPositionComponent>(entity);
			//PRINTLN("entity position x value: {}", worldpos.position.x);
			//registry.destroy(entity);
			//PRINTLN("destorying entity");
		}

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
		// ImGUI draw
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::SetNextWindowSize(ImVec2(500, 400));
		//scene->draw_imgui();
		ui_mananger.draw_object_hierarchy();
		ui_mananger.draw_object_properties();

		ImGui::Begin("UI Manager Window");
		bool focused = ImGui::IsWindowFocused();
		ImGui::Text("Focused: %s", focused ? "Yes" : "No");
		ImGui::End();


		ImGui::Begin("tree view thingy");

		if (ImGui::TreeNode("Tree view"))
		{
			const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
			const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
			static ImGuiTableFlags table_flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;

			static ImGuiTreeNodeFlags tree_node_flags_base = ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull;

			if (ImGui::BeginTable("3ways", 3, table_flags))
			{
				// The first column will use the default _WidthStretch when ScrollX is Off and _WidthFixed when ScrollX is On
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
				ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 12.0f);
				ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 18.0f);
				ImGui::TableHeadersRow();

				// Simple storage to output a dummy file-system.
				struct MyTreeNode
				{
					const char* Name;
					const char* Type;
					int             Size;
					int             ChildIdx;
					int             ChildCount;
					static void DisplayNode(const MyTreeNode* node, const MyTreeNode* all_nodes)
					{
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						const bool is_folder = (node->ChildCount > 0);

						ImGuiTreeNodeFlags node_flags = tree_node_flags_base;
						if (node != &all_nodes[0])
							node_flags &= ~ImGuiTreeNodeFlags_LabelSpanAllColumns; // Only demonstrate this on the root node.

						if (is_folder)
						{
							bool open = ImGui::TreeNodeEx(node->Name, node_flags);
							if ((node_flags & ImGuiTreeNodeFlags_LabelSpanAllColumns) == 0)
							{
								ImGui::TableNextColumn();
								ImGui::TextDisabled("--");
								ImGui::TableNextColumn();
								ImGui::TextUnformatted(node->Type);
							}
							if (open)
							{
								for (int child_n = 0; child_n < node->ChildCount; child_n++)
									DisplayNode(&all_nodes[node->ChildIdx + child_n], all_nodes);
								ImGui::TreePop();
							}
						}
						else
						{
							ImGui::TreeNodeEx(node->Name, node_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen);
							ImGui::TableNextColumn();
							ImGui::Text("%d", node->Size);
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(node->Type);
						}
					}
				};
				static const MyTreeNode nodes[] =
				{
					{ "Root with Long Name",          "Folder",       -1,       1, 3    }, // 0
					{ "Music",                        "Folder",       -1,       4, 2    }, // 1
					{ "Textures",                     "Folder",       -1,       6, 3    }, // 2
					{ "desktop.ini",                  "System file",  1024,    -1,-1    }, // 3
					{ "File1_a.wav",                  "Audio file",   123000,  -1,-1    }, // 4
					{ "File1_b.wav",                  "Audio file",   456000,  -1,-1    }, // 5
					{ "Image001.png",                 "Image file",   203128,  -1,-1    }, // 6
					{ "Copy of Image001.png",         "Image file",   203256,  -1,-1    }, // 7
					{ "Copy of Image001 (Final2).png","Image file",   203512,  -1,-1    }, // 8
				};

				MyTreeNode::DisplayNode(&nodes[0], nodes);

				ImGui::EndTable();
			}
			ImGui::TreePop();
		}

		ImGui::End();

		//ImGui::Begin("test trreee");
		//if (ImGui::TreeNodeEx("node_label", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen))
		//{

		//}
		//
		//if (ImGui::TreeNode("Root")) {
		//	if (ImGui::TreeNode("Child 1")) {
		//		ImGui::Text("Leaf 1");
		//		ImGui::TreePop();
		//	}
		//	if (ImGui::TreeNode("Child 2")) {
		//		ImGui::Text("Leaf 2");
		//		ImGui::TreePop();
		//	}
		//	ImGui::TreePop();

		//}
		//if (ImGui::TreeNode("Root 2"))
		//{
		//	ImGui::TreePop();
		//}
		//

		//ImGui::End();


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
