#include "OD/pch.h"
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
    if(ImGui::BeginTabBar("RendererTabs")){
        if(ImGui::BeginTabItem("General")){
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

            ImGui::EndTabItem();
        }

        if(ImGui::BeginTabItem("GPU Memory")){
            auto& mem = Graphics::GetMemoryStats(); // you will implement this

            ImGui::Text("Mesh: %.2f MB", mem.meshBytes / (1024.0f * 1024.0f));
            ImGui::Text("Textures: %.2f MB", mem.texturesBytes / (1024.0f * 1024.0f));
            ImGui::Text("Framebuffer: %.2f MB", mem.framebuffersBytes / (1024.0f * 1024.0f));
            ImGui::Text("Buffers: %.2f MB", mem.buffersBytes / (1024.0f * 1024.0f));

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    //ImGui::Text("Tris: %dk", Graphics::GetTrisCount() / 1000);
    ImGui::End();
}

}