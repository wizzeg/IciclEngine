#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui-docking/imgui.h>
#include <imgui-docking/imgui_impl_glfw.h>
#include <imgui-docking/imgui_impl_opengl3.h>
#include <string>
#include <vector>
#include <engine/utilities/macros.h>
#include <memory>
#include <functional>


class Scene;

struct MenuItem
{
    MenuItem(const std::string& a_name, const std::string& a_shortcut, std::vector<std::function<void()>>&& funcs) :
        item_name(a_name), shortcut(a_shortcut), functions(std::move(funcs))
    {
    }
    std::string item_name = "file";
    std::string shortcut = "Ctrl+N";
    std::vector<std::function<void()>> functions;
    void draw_menu_item()
    {
        if (ImGui::MenuItem(item_name.c_str(), shortcut.c_str()))
        {
            for (auto& function : functions)
            {
                function();
            }
        }
    }

    virtual void item_execute() { PRINTLN("This is so stupid"); };

};

struct BarTab
{
    BarTab(std::vector<std::function<void()>>&& funcs)
    {
        menu_items.push_back(MenuItem("scene", "Ctrl+N", std::move(funcs)));
    }
    std::string bar_name = "file";
    std::vector<MenuItem> menu_items;
    void draw_bar_tab()
    {
        if (ImGui::BeginMenu(bar_name.c_str()))
        {
            for (auto& item : menu_items)
            {
                item.draw_menu_item();
            }
            ImGui::EndMenu();
        }
        
    }
};

struct UIMenuBar
{
    UIMenuBar(std::vector<BarTab>&& a_tabs) : tabs(a_tabs)
    {
        if (a_tabs.size() == 0)
        {
            PRINTLN("no tabs");
        }
    }
    std::vector<BarTab> tabs;
    void draw_menu_bar()
    {
        int id = 0;
        if (ImGui::BeginMainMenuBar())
        {
            ImGui::PushID(id++);
            for (auto& tab : tabs)
            {
                tab.draw_bar_tab();
            }
            ImGui::PopID();
            ImGui::EndMainMenuBar();
        }
        
    }
};

struct MenuBar
{
    void draw_menu_bar();
    void set_scene(std::weak_ptr<Scene> a_scene);
    std::weak_ptr<Scene> scene;
    bool open_save_popup = false;
    bool open_load_popup = false;
    char scene_path[260] = "scenes/";
    
};

//
//void RenderTopMenuBar() {
//    if (ImGui::BeginMainMenuBar()) {
//        // Your existing menus (unchanged)
//        if (ImGui::BeginMenu("File")) {
//            if (ImGui::MenuItem("New", "Ctrl+N")) {}
//            if (ImGui::MenuItem("Open", "Ctrl+O")) {}
//            if (ImGui::MenuItem("Save", "Ctrl+S")) {}
//            ImGui::Separator();
//            if (ImGui::MenuItem("Exit")) {}
//            ImGui::EndMenu();
//        }
//        if (ImGui::BeginMenu("Edit")) { /* ... */ }
//        if (ImGui::BeginMenu("View")) { /* ... */ }
//        if (ImGui::BeginMenu("Tools")) { /* ... */ }
//
//        // ✅ FIXED: Unique IDs with ## + Plain text
//        float menuWidth = ImGui::GetWindowSize().x;
//        ImGui::SameLine(menuWidth - 180);  // Adjusted for longer text
//
//        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
//        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 3));
//
//        // Play button - UNIQUE ID "Play##MenuBar"
//        if (ImGui::Button(playback.isPlaying && !playback.isPaused ? "Pause##PlayBtn" : "Play##PlayBtn", ImVec2(45, 18))) {
//            if (playback.isPlaying && !playback.isPaused) {
//                playback.isPaused = true;
//            }
//            else {
//                playback.isPlaying = true;
//                playback.isPaused = false;
//            }
//
//            // set playing to true
//
//        }
//
//        ImGui::SameLine();
//
//        // Stop button - UNIQUE ID "Stop##MenuBar"  
//        if (ImGui::Button("Stop##StopBtn", ImVec2(45, 18))) {
//            playback.isPlaying = false;
//            playback.isPaused = false;
//
//            //set playing to false.
//
//        }
//
//        ImGui::PopStyleVar(2);
//        ImGui::EndMainMenuBar();
//    }
//}