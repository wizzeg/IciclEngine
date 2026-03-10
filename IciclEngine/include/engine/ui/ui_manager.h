#pragma once

#include <memory>
#include <engine/ui/ui_scene_hierarchy_drawer.h>
#include <engine/ui/ui_object_property_drawer.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui-docking/imgui.h>
#include <imgui-docking/imgui_impl_glfw.h>
#include <imgui-docking/imgui_impl_opengl3.h>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <engine/core/game_thread.h>
#include <engine/ui/ui_menu_bar.h>

class Scene;
class SceneObject;
struct EngineContext;


// for now slightly modified ai generated
struct PlaybackControls {
    bool isPlaying = false;
    bool isPaused = false;

    void Render() {
        // Transport controls group
        ImGui::BeginGroup();

        // PLAY button ▶️
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        if (ImGui::Button(isPlaying && !isPaused ? "pause" : "play", ImVec2(40, 40))) {
            if (isPlaying && !isPaused) {
                isPaused = true;  // Pause
            }
            else {
                isPlaying = true;
                isPaused = false; // Play
            }
        }
        // REPLACE WITH IMAGE: ImGui::Image((ImTextureID)play_texture_id, ImVec2(32, 32));
        ImGui::PopStyleVar();

        ImGui::SameLine();

        // STOP button ⏹️
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        if (ImGui::Button("stop", ImVec2(40, 40))) {
            isPlaying = false;
            isPaused = false;  // Stop
        }
        // REPLACE WITH IMAGE: ImGui::Image((ImTextureID)stop_texture_id, ImVec2(32, 32));
        ImGui::PopStyleVar();

        ImGui::SameLine();

        // PAUSE button ⏸️ (only show when playing)
        if (isPlaying) {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
            if (ImGui::Button("pause", ImVec2(40, 40))) {
                isPaused = !isPaused;  // Toggle pause
            }
            // REPLACE WITH IMAGE: ImGui::Image((ImTextureID)pause_texture_id, ImVec2(32, 32));
            ImGui::PopStyleVar();
        }

        ImGui::EndGroup();
    }
};

class UIManager : UISceneHierarchyDrawer , UIObjectPropertyDrawer
{
	bool should_draw_object_properties = true;
	bool shoud_draw_object_hierarchy = true;
	bool draw_ui = true;
	std::weak_ptr<Scene> scene;
	std::weak_ptr<SceneObject> prev_selected_scene_object;
	GLuint texture_id = 0;
	//UISceneHierarchyDrawer ui_hiearchy_drawer;
	//UIObjectPropertyDrawer ui_property_drawer;
    PlaybackControls playback;
    MenuBar menu_bar;
public:
    void draw_menubar();
    void draw_systems();
	void draw_object_hierarchy();
	void draw_object_properties();
	void set_scene(std::weak_ptr<Scene> a_scene);
	void set_draw_texture(GLuint a_texture_id);

    int prev_order = 0;

    void render_play_stop(EngineContext* a_engine_context);

};

