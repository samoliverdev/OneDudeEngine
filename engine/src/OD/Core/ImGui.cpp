#include "ImGui.h"
#include "OD/Editor/Editor.h"

namespace ImGui{

void _SelectionAsset(OD::Ref<OD::Asset> asset){
    if(OD::Editor::Get() != nullptr){
        OD::Editor::Get()->SetSelectionAsset(asset);
    }
}

void AcceptFileMovePayload(std::function<void(std::filesystem::path*)> func){
    if(ImGui::BeginDragDropTarget()){
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(FILE_MOVE_PAYLOAD);
        if(payload != nullptr){
            std::filesystem::path* path = (std::filesystem::path*)payload->Data;
            LogInfo("%s %s", path->string().c_str(), path->extension().string().c_str());
            func(path);
        }
        ImGui::EndDragDropTarget();
    }
}

void ColorEdit3(const char* name, OD::Color* color, ImGuiColorEditFlags flags){
    float _color[] = {color->r, color->g, color->b};
    if(ImGui::ColorEdit3("color", _color, flags)){
        *color = OD::Color{_color[0], _color[1], _color[2], 1};
    }
}

void ColorEdit4(const char* name, OD::Color* color, ImGuiColorEditFlags flags){
    float _color[] = {color->r, color->g, color->b, color->a};
    if(ImGui::ColorEdit4("color", _color, flags)){
        *color = OD::Color{_color[0], _color[1], _color[2], _color[3]};
    }
}

}

namespace OD{

bool cleanAll = false;

void ImGuiLayer::SetDarkTheme(){
    ImGuiIO& io = ImGui::GetIO();
    float baseFontSize = 20.0f; // 13.0f is the size of the default font. Change to the font size you use.
    float iconFontSize = baseFontSize * 2.0f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly

    // merge in icons from Font Awesome
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig icons_config; 
    icons_config.MergeMode = true; 
    icons_config.PixelSnapH = true; 
    icons_config.GlyphMinAdvanceX = iconFontSize;
    io.Fonts->AddFontFromFileTTF("Engine/Fonts/fa-solid-900.ttf", iconFontSize, &icons_config, icons_ranges );
    // use FONT_ICON_FILE_NAME_FAR if you want regular instead of solid

    auto& colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

    // Headers
    colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    
    // Buttons
    colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Frame BG
    colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
    colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
    colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

    // Title
    colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
}

void ImGuiLayer::SetCleanAll(bool value){
    cleanAll = value;
}

bool ImGuiLayer::GetCleanAll(){
    return cleanAll;
}

}