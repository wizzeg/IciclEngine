#pragma once

#include "glfw_context.h"

#include <imgui-docking/imgui.h>
#include <imgui-docking/imgui_impl_glfw.h>
#include <imgui-docking/imgui_impl_opengl3.h>

struct ImGuiManager
{
	ImGuiManager(std::weak_ptr<GLFWContext> a_context) : gl_context(a_context)
	{
		if (auto gl = gl_context.lock())
		{
			gl->activate();
			/* Initialize ImGUI */
			 // this needs to be shared
			imgui_context = ImGui::CreateContext();
			io = &ImGui::GetIO();
			io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
			io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
			io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch
			io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;		  // Enable Multi-Viewport / Platform Windows

			ImGui::StyleColorsDark();
			style = &ImGui::GetStyle();
			ImGui_ImplGlfw_InitForOpenGL(gl->get_window(), true);
			ImGui_ImplOpenGL3_Init("#version 460");

			if (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				style->WindowRounding = 0.0f;
				style->Colors[ImGuiCol_WindowBg].w = 1.0f;
			}
			gl->deactivate();
		}
		else PRINTLN("Failed to generate imgui manager");
	};
	~ImGuiManager()
	{
		if (!cleaned_up)
		{
			if (auto gl = gl_context.lock())
			{
				gl->activate();
				ImGui_ImplOpenGL3_Shutdown();
				ImGui_ImplGlfw_Shutdown();
				ImGui::DestroyContext();
				gl->deactivate();
			}
			else
			{
				ImGui_ImplOpenGL3_Shutdown();
				ImGui_ImplGlfw_Shutdown();
				ImGui::DestroyContext();
			}
		}

		PRINTLN("destructor called imgui");
	}

	void new_frame()
	{
		if (auto gl = gl_context.lock())
		{
			gl->activate();
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		}
	}

	void render()
	{
		if (auto gl = gl_context.lock())
		{
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			if (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				// Capture the current GL context
				GLFWwindow* context_backup = glfwGetCurrentContext();

				// Update and Render additional Platform Windows
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();

				// restore current GL context.
				glfwMakeContextCurrent(context_backup);
			}
			gl->deactivate();
		}
	}

	void destroy()
	{
		if (auto gl = gl_context.lock())
		{
			gl->activate();
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
			gl->deactivate();
			cleaned_up = true;
		}
		else
		{
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
			cleaned_up = true;
		}
	}

private:
	std::weak_ptr<GLFWContext> gl_context;
	ImGuiContext* imgui_context;
	ImGuiIO* io;
	ImGuiStyle* style;
	bool cleaned_up = false;
};

