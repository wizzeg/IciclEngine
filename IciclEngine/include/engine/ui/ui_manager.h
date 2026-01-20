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

class Scene;
class SceneObject;
struct EngineContext;

struct PlaybackControls {
    bool isPlaying = false;
    bool isPaused = false;

    void Render() {
        // Transport controls group
        ImGui::BeginGroup();

        // PLAY button ▶️
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        if (ImGui::Button(isPlaying && !isPaused ? "⏸️" : "▶️", ImVec2(40, 40))) {
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
        if (ImGui::Button("⏹️", ImVec2(40, 40))) {
            isPlaying = false;
            isPaused = false;  // Stop
        }
        // REPLACE WITH IMAGE: ImGui::Image((ImTextureID)stop_texture_id, ImVec2(32, 32));
        ImGui::PopStyleVar();

        ImGui::SameLine();

        // PAUSE button ⏸️ (only show when playing)
        if (isPlaying) {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
            if (ImGui::Button("⏸️", ImVec2(40, 40))) {
                isPaused = !isPaused;  // Toggle pause
            }
            // REPLACE WITH IMAGE: ImGui::Image((ImTextureID)pause_texture_id, ImVec2(32, 32));
            ImGui::PopStyleVar();
        }

        ImGui::EndGroup();
    }
};



namespace fs = std::filesystem;

struct ContentBrowser {
    std::string currentPath = "./assets/";
    std::vector<std::string> pathHistory;  // For back button
    int historyIndex = -1;
    float thumbnailSize = 64.0f;
    float padding = 16.0f;
    int selection = -1;

    ContentBrowser() {
        pathHistory.push_back(currentPath);
        historyIndex = 0;
    }

    void Render() {
        // Path display and navigation
        ImGui::BeginChild("PathBar", ImVec2(0, 25), true);
        ImGui::Text("%s", currentPath.c_str());

        // Back button
        if (ImGui::Button("Back") && historyIndex > 0) {
            historyIndex--;
            currentPath = pathHistory[historyIndex];
        }
        ImGui::SameLine();

        if (ImGui::Button("..")) {
            if (fs::exists(currentPath + "../")) {
                GoToPath(fs::canonical(currentPath + "../").string() + "/");
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Home")) {
            GoToPath("./assets/");
        }
        ImGui::EndChild();

        // File list
        if (ImGui::BeginChild("FileList", ImVec2(0, 0), true)) {
            std::vector<fs::path> entries;
            try {
                if (fs::exists(currentPath) && fs::is_directory(currentPath)) {
                    for (auto& entry : fs::directory_iterator(currentPath)) {
                        entries.push_back(entry.path());
                    }
                    std::sort(entries.begin(), entries.end(),
                        [](const fs::path& a, const fs::path& b) {
                            bool aDir = fs::is_directory(a);
                            bool bDir = fs::is_directory(b);
                            return aDir == bDir ? a < b : aDir > bDir;
                        });
                }
            }
            catch (const fs::filesystem_error& e) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error reading directory");
            }

            float cellSize = thumbnailSize + padding;
            float panelWidth = ImGui::GetContentRegionAvail().x;
            int columns = (int)(panelWidth / cellSize);
            if (columns < 1) columns = 1;

            ImGui::Columns(columns, 0, false);

            for (int i = 0; i < entries.size(); i++) {
                const auto& entry = entries[i];
                const auto& filename = entry.filename().string();

                ImGui::PushID(filename.c_str());

                bool isSelected = (selection == i);
                if (isSelected) {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                }

                // ✅ CLICKABLE IMAGE CONTAINER BUTTON
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.3f, 0.5f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

                // Full clickable area (image + name)
                ImGui::Button("##icon", ImVec2(cellSize, thumbnailSize + 20));

                // Icon
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() - cellSize + 4);
                if (fs::is_directory(entry)) {
                    ImGui::Text("📁");
                }
                else {
                    ImGui::Text("📄");
                }

                // Filename below icon
                ImGui::TextWrapped("%s", filename.c_str());

                // ✅ CLICK & DOUBLE-CLICK ON IMAGE CONTAINER
                if (ImGui::IsItemClicked()) {
                    selection = i;
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    if (fs::is_directory(entry)) {
                        GoToPath(fs::canonical(entry).string() + "/");
                    }
                }

                // Cleanup styles
                ImGui::PopStyleVar();
                if (isSelected) {
                    ImGui::PopStyleColor();
                    ImGui::PopStyleVar();
                }
                ImGui::PopStyleColor(3);
                ImGui::PopID();

                ImGui::NextColumn();
            }
            ImGui::Columns(1);
            ImGui::EndChild();
        }

        ImGui::Separator();
        if (selection >= 0) {
            ImGui::Text("Selected: %s", "file info");
        }
    }

private:
    void GoToPath(const std::string& newPath) {
        // Update history
        if (historyIndex < pathHistory.size() - 1) {
            pathHistory.resize(historyIndex + 1);
        }
        pathHistory.push_back(newPath);
        historyIndex++;
        currentPath = newPath;
        selection = -1;
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

	ContentBrowser content_browser;
    PlaybackControls playback;
public:
	void draw_object_hierarchy();
	void draw_object_properties();
	void draw_selected_icon(glm::mat4 a_view, glm::mat4 a_proj);
	void set_scene(std::weak_ptr<Scene> a_scene);
	void set_draw_texture(GLuint a_texture_id);
	void RenderContentBrowser() {
		ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Content Browser", nullptr, ImGuiWindowFlags_NoCollapse)) {
			content_browser.Render();
		}
		ImGui::End();
	}

    void RenderTopMenuBar() {
        if (ImGui::BeginMainMenuBar()) {
            // Your existing menus (unchanged)
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New", "Ctrl+N")) {}
                if (ImGui::MenuItem("Open", "Ctrl+O")) {}
                if (ImGui::MenuItem("Save", "Ctrl+S")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) { /* ... */ }
            if (ImGui::BeginMenu("View")) { /* ... */ }
            if (ImGui::BeginMenu("Tools")) { /* ... */ }

            // ✅ FIXED: Unique IDs with ## + Plain text
            float menuWidth = ImGui::GetWindowSize().x;
            ImGui::SameLine(menuWidth - 180);  // Adjusted for longer text

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 3));

            // Play button - UNIQUE ID "Play##MenuBar"
            if (ImGui::Button(playback.isPlaying && !playback.isPaused ? "Pause##PlayBtn" : "Play##PlayBtn", ImVec2(45, 18))) {
                if (playback.isPlaying && !playback.isPaused) {
                    playback.isPaused = true;
                }
                else {
                    playback.isPlaying = true;
                    playback.isPaused = false;
                }

                // set playing to true

            }

            ImGui::SameLine();

            // Stop button - UNIQUE ID "Stop##MenuBar"  
            if (ImGui::Button("Stop##StopBtn", ImVec2(45, 18))) {
                playback.isPlaying = false;
                playback.isPaused = false;

                //set playing to false.
               
            }

            ImGui::PopStyleVar(2);
            ImGui::EndMainMenuBar();
        }
    }

    void render_play_stop(EngineContext* a_engine_context);


    void RenderToolbar() {
        // ✅ SIMPLIFIED - No complex positioning
        ImGui::Begin("Toolbar", nullptr,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);

        ImGui::SetWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 40));

        // Center playback controls
        float centerX = (ImGui::GetWindowSize().x - 140) * 0.5f;
        ImGui::SetCursorPosX(centerX);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

        // Play/Pause
        if (ImGui::Button(playback.isPlaying && !playback.isPaused ? "⏸️" : "▶️", ImVec2(40, 30))) {
            if (playback.isPlaying && !playback.isPaused) {
                playback.isPaused = true;
            }
            else {
                playback.isPlaying = true;
                playback.isPaused = false;
            }
        }
        ImGui::SameLine();

        // Stop
        if (ImGui::Button("⏹️", ImVec2(40, 30))) {
            playback.isPlaying = false;
            playback.isPaused = false;
        }

        ImGui::PopStyleVar();
        ImGui::End();
    }

};

