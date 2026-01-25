#include <engine/ui/ui_menu_bar.h>
#include <engine/editor/scene.h>

void MenuBar::draw_menu_bar()
{
    int id = 0;
    if (ImGui::BeginMainMenuBar())
    {
        ImGui::PushID(id++);
        if (ImGui::BeginMenu("Scene"))
        {
            if (ImGui::MenuItem("new scene", "Ctrl+N"))
            {
                if (auto scn = scene.lock())
                {
                    //scn->new_scene_object(("new scene object " + scn->get_next_index()), true);
                    scn->reset();
                }
            }
            if (ImGui::MenuItem("load scene", "Ctrl+O"))
            {
                if (auto scn = scene.lock())
                {
                    open_load_popup = true;
                    //ImGui::OpenPopup("Load Scene at ...");
                }
            }
            if (ImGui::MenuItem("save scene as", "Ctrl+S"))
            {
                if (auto scn = scene.lock())
                {
                    open_save_popup = true;
                    //ImGui::OpenPopup("Save Scene at ...");
                }
            }
            ImGui::EndMenu();
        }

        ImGui::PopID();
        ImGui::EndMainMenuBar();
    }

    if (open_load_popup)
    {
        ImGui::OpenPopup("Load Scene at ...");
    }
    if (open_save_popup)
    {
        ImGui::OpenPopup("Save Scene As ...");
    }

    if (ImGui::BeginPopupModal("Load Scene at ...", &open_load_popup, ImGuiWindowFlags_AlwaysAutoResize))
    {

        ImGui::Text("path: ");
        ImGui::SameLine();
        ImGui::InputText("##Path", scene_path, IM_ARRAYSIZE(scene_path));
        std::string full_path = "./assets/" + std::string(scene_path) + ".scn";
        ImGui::Text(full_path.c_str());

        if (ImGui::Button("Load", ImVec2(120, 0)))
        {
            open_load_popup = false;
            if (auto scn = scene.lock())
                scn->load(full_path, true);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            open_load_popup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Save Scene As ...", &open_save_popup, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("path: ");
        ImGui::SameLine();
        ImGui::InputText("##Path", scene_path, IM_ARRAYSIZE(scene_path));
        std::string full_path = "./assets/" + std::string(scene_path) + ".scn";
        ImGui::Text(full_path.c_str());
        if (ImGui::Button("Save", ImVec2(120, 0)))
        {
            open_save_popup = false;
            if (auto scn = scene.lock())
                scn->save(full_path);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            open_save_popup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void MenuBar::set_scene(std::weak_ptr<Scene> a_scene)
{
    scene = a_scene;
}