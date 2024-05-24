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
        
        if(Graphics::GetVerticesCount() >= 1000000){
            ImGui::Text("Vertices: %.1fM", Graphics::GetVerticesCount() / 1000000.0f);
        } else if(Graphics::GetVerticesCount() >= 1000){
            ImGui::Text("Vertices: %.1fk", Graphics::GetVerticesCount() / 1000.0f);
        } else {
            ImGui::Text("Vertices: %d", Graphics::GetVerticesCount());
        }

        if(Graphics::GetTrisCount() >= 1000000){
            ImGui::Text("Tris: %.1fM", Graphics::GetTrisCount() / 1000000.0f);
        } else if(Graphics::GetTrisCount() >= 1000){
            ImGui::Text("Tris: %.1fk", Graphics::GetTrisCount() / 1000.0f);
        } else {
            ImGui::Text("Tris: %d", Graphics::GetTrisCount());
        }

        //ImGui::Text("Tris: %dk", Graphics::GetTrisCount() / 1000);
        ImGui::End();
    }
};

}