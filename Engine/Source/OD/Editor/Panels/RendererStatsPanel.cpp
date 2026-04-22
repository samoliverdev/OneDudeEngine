#include "OD/pch.h"
#include "RendererStatsPanel.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/SubShader.h"
#include "OD/Graphics/Material.h"
#include "OD/Graphics/Framebuffer.h"
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
            ImGui::Text("uniformBufferUpdates: %d", Graphics::GetStats().uniformBufferUpdates);
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

        /*
        if(ImGui::BeginTabItem("Debug")){
            auto& debug = Graphics::GetGraphicsDebug(); // your global or frame debug

            SubShader* currentShader = nullptr;
            Material* currentMaterial = nullptr;

            int id = 0;

            for(const auto& cmd : debug.datas){
                ImGui::PushID(id++);

                switch(cmd.type){
                    case GraphicsDebug::Type::BindShader:{
                        currentShader = cmd.bindSubShader;

                        std::string label = "Shader: ";
                        label += currentShader ? currentShader->name : "null";

                        if(ImGui::TreeNode(label.c_str())){
                            ImGui::TreePop();
                        }
                        break;
                    }

                    case GraphicsDebug::Type::BindMaterial:{
                        currentMaterial = cmd.bindMaterial;

                        std::string label = "Material: ";
                        label += currentMaterial ? currentMaterial->Path() : "null";

                        if(ImGui::TreeNode(label.c_str())){
                            ImGui::TreePop();
                        }
                        break;
                    }

                    case GraphicsDebug::Type::DrawMesh:{
                        Mesh* mesh = cmd.drawMesh;

                        ImGui::BulletText("DrawMesh: %s (%d tris)",
                            mesh ? mesh->Path() : "null",
                            mesh ? mesh->IndiceCount() / 3 : 0
                        );
                        break;
                    }

                    case GraphicsDebug::Type::DrawMeshSkinned:{
                        Mesh* mesh = cmd.drawMeshSkinned;

                        ImGui::BulletText("DrawMeshSkinned: %s",
                            mesh ? mesh->Path() : "null"
                        );
                        break;
                    }

                    case GraphicsDebug::Type::DrawMeshInstancing:{
                        Mesh* mesh = cmd.drawMeshSkinned;

                        ImGui::BulletText("DrawMeshInstanced: %s",
                            mesh ? mesh->Path() : "null"
                        );
                        break;
                    }

                    case GraphicsDebug::Type::BeginPass:{
                        auto* data = cmd.pass;
                        ImGui::BulletText("BeginPass: %s",
                            data ? data->name.c_str() : "null"
                        );
                        break;
                    }

                    case GraphicsDebug::Type::EndPass:{
                        ImGui::BulletText("EndPass");
                        break;
                    }
                }

                ImGui::PopID();
            }

            ImGui::EndTabItem();
        }
        */

        if(ImGui::BeginTabItem("Debug(Experimental)")){
            auto& debug = Graphics::GetGraphicsDebug();

            SubShader* currentShader = nullptr;
            Material* currentMaterial = nullptr;

            int id = 0;
            bool passOpen = true;

            for(const auto& cmd : debug.datas){
                ImGui::PushID(id++);

                switch(cmd.type){
                    // BEGIN PASS (collapsible)
                    case GraphicsDebug::Type::BeginPass:{
                        auto* data = cmd.pass;

                        std::string label = "Pass: ";
                        label += data ? data->name : "null";

                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));

                        passOpen = ImGui::TreeNodeEx(
                            label.c_str()
                        );

                        ImGui::PopStyleColor();

                        break;
                    }

                    // END PASS
                    case GraphicsDebug::Type::EndPass:{
                        if(passOpen){
                            ImGui::TreePop();
                        }
                        break;
                    }

                    // Skip content if collapsed
                    default:{
                        if(!passOpen){
                            ImGui::PopID();
                            continue;
                        }

                        switch(cmd.type){
                            case GraphicsDebug::Type::BindShader:{
                                currentShader = cmd.bindSubShader;

                                std::string label = "Shader: ";
                                label += currentShader ? currentShader->name.c_str() : "null",

                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
                                ImGui::TreeNodeEx(
                                    label.c_str(), // currentShader ? currentShader->name.c_str() : "Shader: null",
                                    ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen
                                );
                                ImGui::PopStyleColor();
                                break;
                            }

                            case GraphicsDebug::Type::BindMaterial:{
                                currentMaterial = cmd.bindMaterial;

                                std::string label = "Material: ";
                                label += currentMaterial ? currentMaterial->Path() : "null";

                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 1.0f, 0.6f, 1.0f));
                                ImGui::TreeNodeEx(
                                    label.c_str(),
                                    ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen
                                );
                                ImGui::PopStyleColor();
                                break;
                            }

                            case GraphicsDebug::Type::UniformSet:{
                                const char* n = cmd.uniformSet;

                                std::string label = "UniformSet: ";
                                label += n ? n : "null";

                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                                ImGui::TreeNodeEx(
                                    label.c_str(),
                                    ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen
                                );
                                ImGui::PopStyleColor();
                                break;
                            }

                            case GraphicsDebug::Type::DrawMesh:{
                                Mesh* mesh = cmd.drawMesh;

                                std::string label = "DrawMesh: ";
                                label += mesh ? mesh->Path() : "null";
                                label += " (" + std::to_string(mesh ? mesh->IndiceCount() / 3 : 0) + " tris)";

                                ImGui::TreeNodeEx(
                                    label.c_str(),
                                    ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen
                                );
                                break;
                            }

                            case GraphicsDebug::Type::DrawMeshSkinned:{
                                Mesh* mesh = cmd.drawMeshSkinned;

                                std::string label = "DrawMeshSkinned: ";
                                label += mesh ? mesh->Path() : "null";

                                ImGui::TreeNodeEx(
                                    label.c_str(),
                                    ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen
                                );
                                break;
                            }

                            case GraphicsDebug::Type::DrawMeshInstancing:{
                                Mesh* mesh = cmd.drawMesh;

                                std::string label = "DrawMeshInstanced: ";
                                label += mesh ? mesh->Path() : "null";

                                ImGui::TreeNodeEx(
                                    label.c_str(),
                                    ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen
                                );
                                break;
                            }

                            default: break;
                        }

                        break;
                    }
                }

                ImGui::PopID();
            }

            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    //ImGui::Text("Tris: %dk", Graphics::GetTrisCount() / 1000);
    ImGui::End();
}

}