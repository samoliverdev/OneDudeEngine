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
    ImGui::Text("DrawCalls: %d", Graphics::GetStats().drawCalls);
    ImGui::Text("UniformSets: %d", Graphics::GetStats().uniformSet);
    ImGui::Text("ShaderBinds: %d", Graphics::GetStats().shaderBinds);
    ImGui::Text("MaterialSubmitDatas: %d", Graphics::GetStats().materialSubmitDatas);
    
    if(Graphics::GetStats().vertices >= 1000000){
        ImGui::Text("Vertices: %.1fM", Graphics::GetStats().vertices / 1000000.0f);
    } else if(Graphics::GetStats().vertices >= 1000){
        ImGui::Text("Vertices: %.1fk", Graphics::GetStats().vertices / 1000.0f);
    } else {
        ImGui::Text("Vertices: %d", Graphics::GetStats().vertices);
    }

    if(Graphics::GetStats().tris >= 1000000){
        ImGui::Text("Tris: %.1fM", Graphics::GetStats().tris / 1000000.0f);
    } else if(Graphics::GetStats().tris >= 1000){
        ImGui::Text("Tris: %.1fk", Graphics::GetStats().tris / 1000.0f);
    } else {
        ImGui::Text("Tris: %d", Graphics::GetStats().tris);
    }

    //ImGui::Text("Tris: %dk", Graphics::GetTrisCount() / 1000);
    ImGui::End();
}

}