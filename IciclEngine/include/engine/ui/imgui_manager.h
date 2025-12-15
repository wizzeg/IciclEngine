#pragma once

#include <engine/renderer/glfw_context.h>

#include <imgui-docking/imgui.h>
#include <imgui-docking/imgui_impl_glfw.h>
#include <imgui-docking/imgui_impl_opengl3.h>
#include <imgui-docking/imgui_internal.h>

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

			ImGui::StyleColorsClassic();
			style = &ImGui::GetStyle();
			style->FrameRounding = 4;
			style->TreeLinesRounding = 4;
			style->ImageBorderSize = 1;
			style->FrameBorderSize = 1;
			style->TreeLinesSize = 2;
			style->GrabRounding = 4;
			style->ChildRounding = 4;

			/*style = &ImGui::GetStyle();

			style->FontScaleMain = 1.0f;

			style->Alpha = 1.0f;
			style->DisabledAlpha = 1.0f;
			style->WindowPadding = ImVec2(12.0f, 12.0f);
			style->WindowRounding = 0.0f;
			style->WindowBorderSize = 0.0f;
			style->WindowMinSize = ImVec2(20.0f, 20.0f);
			style->WindowTitleAlign = ImVec2(0.5f, 0.5f);
			style->WindowMenuButtonPosition = ImGuiDir_None;
			style->ChildRounding = 0.0f;
			style->ChildBorderSize = 1.0f;
			style->PopupRounding = 0.0f;
			style->PopupBorderSize = 1.0f;
			style->FramePadding = ImVec2(6.0f, 6.0f);
			style->FrameRounding = 0.0f;
			style->FrameBorderSize = 0.0f;
			style->ItemSpacing = ImVec2(12.0f, 6.0f);
			style->ItemInnerSpacing = ImVec2(6.0f, 3.0f);
			style->CellPadding = ImVec2(12.0f, 6.0f);
			style->IndentSpacing = 20.0f;
			style->ColumnsMinSpacing = 6.0f;
			style->ScrollbarSize = 12.0f;
			style->ScrollbarRounding = 0.0f;
			style->GrabMinSize = 12.0f;
			style->GrabRounding = 0.0f;
			style->TabRounding = 0.0f;
			style->TabBorderSize = 0.0f;
			style->TabCloseButtonMinWidthSelected = 0.0f;
			style->TabCloseButtonMinWidthUnselected = 0.0f;
			style->ColorButtonPosition = ImGuiDir_Right;
			style->ButtonTextAlign = ImVec2(0.5f, 0.5f);
			style->SelectableTextAlign = ImVec2(0.0f, 0.5f);
			style->TableAngledHeadersTextAlign = ImVec2(0.0f, 0.5f);


			style->Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.27450982f, 0.31764707f, 0.4509804f, 1.0f);
			style->Colors[ImGuiCol_WindowBg] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
			style->Colors[ImGuiCol_ChildBg] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
			style->Colors[ImGuiCol_PopupBg] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
			style->Colors[ImGuiCol_Border] = ImVec4(0.15686275f, 0.16862746f, 0.19215687f, 1.0f);
			style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
			style->Colors[ImGuiCol_FrameBg] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
			style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15686275f, 0.16862746f, 0.19215687f, 1.0f);
			style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.23529412f, 0.21568628f, 0.59607846f, 1.0f);
			style->Colors[ImGuiCol_TitleBg] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
			style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
			style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
			style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.09803922f, 0.105882354f, 0.12156863f, 1.0f);
			style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
			style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
			style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.15686275f, 0.16862746f, 0.19215687f, 1.0f);
			style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
			style->Colors[ImGuiCol_CheckMark] = ImVec4(0.49803922f, 0.5137255f, 1.0f, 1.0f);
			style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.49803922f, 0.5137255f, 1.0f, 1.0f);
			style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5372549f, 0.5529412f, 1.0f, 1.0f);
			style->Colors[ImGuiCol_Button] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
			style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.19607843f, 0.1764706f, 0.54509807f, 1.0f);
			style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.23529412f, 0.21568628f, 0.59607846f, 1.0f);
			style->Colors[ImGuiCol_Header] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
			style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.19607843f, 0.1764706f, 0.54509807f, 1.0f);
			style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.23529412f, 0.21568628f, 0.59607846f, 1.0f);
			style->Colors[ImGuiCol_Separator] = ImVec4(0.15686275f, 0.18431373f, 0.2509804f, 1.0f);
			style->Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.15686275f, 0.18431373f, 0.2509804f, 1.0f);
			style->Colors[ImGuiCol_SeparatorActive] = ImVec4(0.15686275f, 0.18431373f, 0.2509804f, 1.0f);
			style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
			style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.19607843f, 0.1764706f, 0.54509807f, 1.0f);
			style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.23529412f, 0.21568628f, 0.59607846f, 1.0f);
			style->Colors[ImGuiCol_Tab] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
			style->Colors[ImGuiCol_TabHovered] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
			style->Colors[ImGuiCol_TabActive] = ImVec4(0.09803922f, 0.105882354f, 0.12156863f, 1.0f);
			style->Colors[ImGuiCol_TabUnfocused] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
			style->Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
			style->Colors[ImGuiCol_PlotLines] = ImVec4(0.52156866f, 0.6f, 0.7019608f, 1.0f);
			style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.039215688f, 0.98039216f, 0.98039216f, 1.0f);
			style->Colors[ImGuiCol_PlotHistogram] = ImVec4(1.0f, 0.2901961f, 0.59607846f, 1.0f);
			style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.99607843f, 0.4745098f, 0.69803923f, 1.0f);
			style->Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
			style->Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
			style->Colors[ImGuiCol_TableBorderLight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style->Colors[ImGuiCol_TableRowBg] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
			style->Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.09803922f, 0.105882354f, 0.12156863f, 1.0f);
			style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.23529412f, 0.21568628f, 0.59607846f, 1.0f);
			style->Colors[ImGuiCol_DragDropTarget] = ImVec4(0.49803922f, 0.5137255f, 1.0f, 1.0f);
			style->Colors[ImGuiCol_NavHighlight] = ImVec4(0.49803922f, 0.5137255f, 1.0f, 1.0f);
			style->Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.49803922f, 0.5137255f, 1.0f, 1.0f);
			style->Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.19607843f, 0.1764706f, 0.54509807f, 0.5019608f);
			style->Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.25f, 0.25f, 0.25f, 0.5019608f);*/

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

	void make_dockspace()
	{
		// Get the main viewport (covers the entire window)
		ImGuiViewport* viewport = ImGui::GetMainViewport();

		// Set up window flags - this window will be invisible and cover the entire viewport
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoBringToFrontOnFocus
			| ImGuiWindowFlags_NoNavFocus
			| ImGuiWindowFlags_NoBackground;  // Important: makes the window transparent

		// Dockspace flags
		ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
		// Optional: ImGuiDockNodeFlags_PassthruCentralNode allows clicking through empty space

		// Set the window to cover the entire viewport
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);

		// Remove padding and rounding
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		// Create the invisible window that hosts the dockspace
		ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
		ImGui::PopStyleVar(3);

		// Create the dockspace
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

		ImGui::End();
	}

	// Optional: Call this once on first run to set up initial layout
	void setup_default_docking_layout()
	{
		static bool first_time = true;
		if (!first_time)
			return;
		first_time = false;

		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");

		// Clear any existing layout
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

		// Split the dockspace into sections (example: left and right)
		ImGuiID dock_left, dock_right;
		ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.3f, &dock_left, &dock_right);

		// Dock windows to specific sections
		ImGui::DockBuilderDockWindow("Left Panel", dock_left);
		ImGui::DockBuilderDockWindow("Right Panel", dock_right);

		ImGui::DockBuilderFinish(dockspace_id);
	}

	ImGuiIO* get_io() const { return io; }
private:
	std::weak_ptr<GLFWContext> gl_context;
	ImGuiContext* imgui_context;
	ImGuiIO* io;
	ImGuiStyle* style;
	bool cleaned_up = false;
};

