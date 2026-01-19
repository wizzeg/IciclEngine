//#define IMGUI_DEFINE_MATH_OPERATORS
//#include <imgui-docking/imgui.h>
//#include <imgui-docking/imgui_impl_glfw.h>
//#include <imgui-docking/imgui_impl_opengl3.h>
//#include <filesystem>
//#include <vector>
//#include <string>
//#include <algorithm>
//
//namespace fs = std::filesystem;
//
//struct ContentBrowser {
//    std::string currentPath = "./assets/";
//    float thumbnailSize = 64.0f;
//    float padding = 16.0f;
//    int selection = -1;
//
//    void Render() {
//        // Path display and navigation
//        ImGui::BeginChild("PathBar", ImVec2(0, 25), true);
//        ImGui::Text("%s", currentPath.c_str());
//
//        // Navigation buttons
//        if (ImGui::Button("..")) {
//            if (fs::exists(currentPath + "../")) {
//                currentPath = fs::canonical(currentPath + "../").string() + "/";
//            }
//        }
//        ImGui::SameLine();
//        if (ImGui::Button("Home")) {
//            currentPath = "./assets/";
//        }
//        if (ImGui::Button("Refresh")) {
//            // Force refresh handled by scanning below
//        }
//        ImGui::EndChild();
//
//        // File list
//        if (ImGui::BeginChild("FileList", ImVec2(0, 0), true)) {
//            // Scan directory
//            std::vector<fs::path> entries;
//            try {
//                if (fs::exists(currentPath) && fs::is_directory(currentPath)) {
//                    for (auto& entry : fs::directory_iterator(currentPath)) {
//                        entries.push_back(entry.path());
//                    }
//                    std::sort(entries.begin(), entries.end(),
//                        [](const fs::path& a, const fs::path& b) {
//                            bool aDir = fs::is_directory(a);
//                            bool bDir = fs::is_directory(b);
//                            return aDir == bDir ? a < b : aDir > bDir; // Dirs first
//                        });
//                }
//            }
//            catch (const fs::filesystem_error& e) {
//                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error reading directory");
//            }
//
//            // Calculate layout
//            float cellSize = thumbnailSize + padding;
//            float panelWidth = ImGui::GetContentRegionAvail().x;
//            int columns = (int)(panelWidth / cellSize);
//            if (columns < 1) columns = 1;
//
//            ImGui::Columns(columns, 0, false);
//
//            for (int i = 0; i < entries.size(); i++) {
//                const auto& entry = entries[i];
//                const auto& filename = entry.filename().string();
//
//                // Directory or file thumbnail
//                ImGui::PushID(filename.c_str());
//
//                // Selection
//                bool isSelected = (selection == i);
//                if (isSelected) {
//                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
//                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
//                }
//
//                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
//                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));
//                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.3f, 0.5f, 1.0f));
//                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
//
//                ImGui::Button("##icon", ImVec2(cellSize, thumbnailSize));
//
//                // Thumbnail/Icon
//                if (fs::is_directory(entry)) {
//                    ImGui::SameLine();
//                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - cellSize);
//                    ImGui::Text("📁"); // Folder icon
//                }
//                else {
//                    ImGui::SameLine();
//                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - cellSize);
//                    ImGui::Text("📄"); // File icon
//                }
//
//                // Name
//                ImGui::TextWrapped("%s", filename.c_str());
//
//                // Interactions
//                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
//                    if (fs::is_directory(entry)) {
//                        currentPath = fs::canonical(entry).string() + "/";
//                        selection = -1;
//                    }
//                }
//
//                if (ImGui::IsItemClicked()) {
//                    selection = i;
//                }
//
//                ImGui::PopStyleVar();
//                if (isSelected) {
//                    ImGui::PopStyleColor();
//                    ImGui::PopStyleVar();
//                }
//                ImGui::PopStyleColor(3);
//                ImGui::PopID();
//
//                ImGui::NextColumn();
//            }
//
//            ImGui::Columns(1);
//            ImGui::EndChild();
//        }
//
//        // Selected file info
//        ImGui::Separator();
//        if (selection >= 0) {
//            // Show selected file info here
//            ImGui::Text("Selected: %s", "file info");
//        }
//    }
//};