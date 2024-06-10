#pragma once

#include "OD/Defines.h"
#include <imgui/imgui.h>
#include <filesystem>

namespace ImGui{
    void OD_API AcceptFileMovePayload(std::function<void(std::filesystem::path*)> func);
}

namespace OD{

class OD_API ImGuiLayer{
public:    
    static void SetDarkTheme();
    static void SetCleanAll(bool value);
    static bool GetCleanAll();
};

}