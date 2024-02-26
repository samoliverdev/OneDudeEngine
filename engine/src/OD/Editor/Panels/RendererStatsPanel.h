#pragma once
#include "OD/Defines.h"
#include "OD/Editor/EditorPanel.h"
#include "OD/Graphics/Graphics.h"

namespace OD{

class OD_API RendererStatsPanel: public EditorPanel{
public: 
    RendererStatsPanel(){
        name = "RendererStatsPanel";
        show = true;
    }

    inline void OnGui() override {
        ImGui::Begin("Renderer Stats");
        ImGui::Text("DrawCalls: %d", Graphics::GetDrawCallsCount());
        ImGui::Text("Vertices: %dk", Graphics::GetVerticesCount() / 1000);
        ImGui::Text("Tris: %dk", Graphics::GetTrisCount() / 1000);
        ImGui::End();
    }
};

}