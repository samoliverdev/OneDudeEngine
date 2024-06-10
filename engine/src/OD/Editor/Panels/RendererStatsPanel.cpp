#include "RendererStatsPanel.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Core/ImGui.h"

namespace OD{

RendererStatsPanel::RendererStatsPanel(){
    name = "RendererStatsPanel";
    show = true;
}

void RendererStatsPanel::OnGui() {
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

}