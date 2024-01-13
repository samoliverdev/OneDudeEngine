#pragma once

#include "OD/Editor/EditorPanel.h"
#include "OD/Graphics/Graphics.h"

namespace OD{

class RendererStatsPanel: public EditorPanel{
public: 
    RendererStatsPanel(){
        name = "RendererStatsPanel";
        show = true;
    }

    inline void OnGui() override {
        ImGui::Begin("Renderer Stats");
        ImGui::Text("DrawCalls: %d", Graphics::drawCalls);
        ImGui::Text("Vertices: %dk", Graphics::vertices / 1000);
        ImGui::Text("Tris: %dk", Graphics::tris / 1000);
        ImGui::End();
    }
};

}