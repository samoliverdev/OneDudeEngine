#pragma once

#include <imgui/imgui.h>
#include <functional>
#include <filesystem>
#include "OD/Defines.h"

namespace ImGui{
    void AcceptFileMovePayload(std::function<void(std::filesystem::path*)> func);
}

namespace OD{

class ImGuiLayer{
public:    
    static void SetDarkTheme();

    static void SetCleanAll(bool value);
    static bool GetCleanAll();
};

}