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

    auto FileExists = [](const std::string& name){
        std::ifstream f(name);
        return f.good();
    };

    // merge in icons from Font Awesome
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig icons_config; 
    icons_config.MergeMode = true; 
    icons_config.PixelSnapH = true; 
    icons_config.GlyphMinAdvanceX = iconFontSize;
    if(FileExists("Engine/Fonts/fa-solid-900.ttf")){
        io.Fonts->AddFontFromFileTTF("Engine/Fonts/fa-solid-900.ttf", iconFontSize, &icons_config, icons_ranges);
    }
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

    //////////////////////////
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    
    // Colors (inspired by Godot minimal theme's clean dark look)
    style.Colors[ImGuiCol_Text]                  = ImVec4(0.90f, 0.90f, 0.90f, 1.00f); // Light gray text
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f); // Subtle disabled text
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.10f, 0.10f, 0.10f, 1.00f); // Dark background
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.12f, 0.12f, 0.12f, 1.00f); // Slightly lighter child bg
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.15f, 0.15f, 0.15f, 0.95f); // Popup slightly lighter
    style.Colors[ImGuiCol_Border]                = ImVec4(0.20f, 0.20f, 0.20f, 0.50f); // Subtle borders
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // No shadow
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.18f, 0.18f, 0.18f, 0.54f); // Frame background
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.25f, 0.25f, 0.25f, 0.40f); // Hover effect
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.30f, 0.30f, 0.30f, 0.67f); // Active effect
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.08f, 0.08f, 0.08f, 1.00f); // Dark title bar
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.12f, 0.12f, 0.12f, 1.00f); // Active title
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.00f, 0.00f, 0.00f, 0.51f); // Collapsed title
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.14f, 0.14f, 0.14f, 1.00f); // Menu bar
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.10f, 0.10f, 0.10f, 0.53f); // Scrollbar bg
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.31f, 0.31f, 0.31f, 1.00f); // Scrollbar grab
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.41f, 0.41f, 0.41f, 1.00f); // Hover
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.51f, 0.51f, 0.51f, 1.00f); // Active
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.80f, 0.80f, 0.80f, 1.00f); // Checkmark
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.40f, 0.40f, 0.40f, 1.00f); // Slider
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.50f, 0.50f, 0.50f, 1.00f); // Active slider
    style.Colors[ImGuiCol_Button]                = ImVec4(0.20f, 0.20f, 0.20f, 0.40f); // Button
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.30f, 0.30f, 0.30f, 1.00f); // Hover
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.40f, 0.40f, 0.40f, 1.00f); // Active
    style.Colors[ImGuiCol_Header]                = ImVec4(0.20f, 0.20f, 0.20f, 0.31f); // Header
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.30f, 0.30f, 0.30f, 0.80f); // Hover
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.40f, 0.40f, 0.40f, 1.00f); // Active
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.20f, 0.20f, 0.20f, 0.50f); // Separator
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.30f, 0.30f, 0.30f, 0.78f); // Hover
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.40f, 0.40f, 0.40f, 1.00f); // Active
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.20f, 0.20f, 0.20f, 0.25f); // Resize grip
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.30f, 0.30f, 0.30f, 0.67f); // Hover
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.40f, 0.40f, 0.40f, 0.95f); // Active
    style.Colors[ImGuiCol_Tab]                   = ImVec4(0.15f, 0.15f, 0.15f, 0.86f); // Tab
    style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.30f, 0.30f, 0.30f, 0.80f); // Hover
    style.Colors[ImGuiCol_TabActive]             = ImVec4(0.25f, 0.25f, 0.25f, 1.00f); // Active
    style.Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.12f, 0.12f, 0.12f, 0.97f); // Unfocused
    style.Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.18f, 0.18f, 0.18f, 1.00f); // Unfocused active
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.61f, 0.61f, 0.61f, 1.00f); // Plot lines
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f); // Hover (slight color accent)
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f); // Histogram
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f); // Hover
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f); // Selection
    style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 1.00f, 0.00f, 0.90f); // Drag drop
    style.Colors[ImGuiCol_NavHighlight]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f); // Nav highlight
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f); // Windowing highlight
    style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f); // Dim background
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f); // Modal dim

    // Style adjustments for minimalism
    style.WindowRounding    = 0.0f;  // Sharp corners
    style.FrameRounding     = 0.0f;  // Sharp frames
    style.PopupRounding     = 0.0f;  // Sharp popups
    style.ScrollbarRounding = 0.0f;  // Sharp scrollbars
    style.GrabRounding      = 0.0f;  // Sharp grabs
    style.TabRounding       = 0.0f;  // Sharp tabs
    style.WindowBorderSize  = 1.0f;  // Thin borders
    style.FrameBorderSize   = 0.0f;  // No frame borders
    style.PopupBorderSize   = 1.0f;  // Thin popup borders
    style.ChildBorderSize   = 1.0f;  // Thin child borders
    style.Alpha             = 1.0f;  // Full opacity
    style.ItemSpacing       = ImVec2(8.0f, 4.0f); // Clean spacing
    style.WindowPadding     = ImVec2(8.0f, 8.0f); // Balanced padding
}

void ImGuiLayer::SetCleanAll(bool value){
    cleanAll = value;
}

bool ImGuiLayer::GetCleanAll(){
    return cleanAll;
}

}