#include "OD/pch.h"
#include "RuntimeInfoPanel.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/Application.h"
#include "OD/Scene/SceneManager.h"

namespace OD{

RuntimeInfoPanel::RuntimeInfoPanel(){
    name = "RuntimeInfoPanel";
    show = true;
}

void RuntimeInfoPanel::OnGui(){
    auto DrawSystems = [](const char* name, const std::vector<System*> systems){
        if(ImGui::TreeNodeEx(name)){
            for(auto* i : systems){
                std::string _name = i->Name(); 
                ImGui::TreeNodeEx(_name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
            }
            ImGui::TreePop();
        }
    };
    
    if(ImGui::Begin("RuntimeInfo", &show)){

        // Modules
        if(ImGui::TreeNodeEx("Modules")){
            for(auto* i : Application::Modules()){
                ImGui::TreeNodeEx(i->Name().c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);// Leaf node: NO TreePop!
            }
            ImGui::TreePop(); // OK: only once
        }

        ///////////////////////////////////////
        Scene* scene = SceneManager::Get().GetActiveScene();
        if(scene != nullptr){

            if(ImGui::TreeNodeEx("Active Scene")){
                DrawSystems("Stand Systems", scene->GetStandSystems());
                DrawSystems("Animation Systems", scene->GetAnimationSystems());
                DrawSystems("Pre Physics Systems", scene->GetPrePhysicsSystems());
                DrawSystems("Fixed Physics Systems", scene->GetFixedPhysicsSystems());
                DrawSystems("Post Physics Systems", scene->GetPostPhysicsSystems());
                DrawSystems("Later Systems", scene->GetLateSystems());
                DrawSystems("Renderer Systems", scene->GetRendererSystems());
                ImGui::TreePop();
            }
        }
    }
    ImGui::End();
}

}