#pragma once

#include <imgui/imgui.h>

namespace OD{

class ImGuiLayer{
public:    
    static void SetDarkTheme();

    static void SetCleanAll(bool value);
    static bool GetCleanAll();
};

}