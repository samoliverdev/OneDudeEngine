#include "ProfilePanel.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Core/ImGui.h"

namespace OD{

ProfilePanel::ProfilePanel(){
    name = "ProfilePanel";
    show = true;
}

std::vector<int> GetChildrens(unsigned int index){
    auto results = Instrumentor::Results();
    std::vector<int> out;
    
    for(int i = 0; i < results.size(); i++){
        if(results[i].parent == index) out.push_back(i);
    }

    return out;
}

void DrawNode(int index){
    auto results = Instrumentor::Results();
    ProfileResult& result = results[index];
    std::vector<int> ch = GetChildrens(index);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
    if(ch.size() == 0) flags |= ImGuiTreeNodeFlags_Leaf;

    float durration = (result.end - result.start) * 0.001f;
    if(ImGui::TreeNodeEx(result.name, flags, "%s: %.3f.ms", result.name, durration)){
        for(auto i: ch){
            DrawNode(i);
        }
        ImGui::TreePop();
    }
}

void ProfilePanel::OnGui(){
    if(ImGui::Begin("Profile", &show)){
        ImGui::DrawEnumCombo<ViewMode>("ViewMode", &viewMode);

        ImGui::Text("Results Count:: %zd", Instrumentor::Results().size());

        ImGui::Separator();

        if(viewMode == ViewMode::List){
            for(auto i: Instrumentor::Results()){ 
                float durration = (i.end - i.start) * 0.001f;
                ImGui::Text("%s: %.3f.ms", i.name, durration);
            }
        }

        if(viewMode == ViewMode::Tree && Instrumentor::Results().size() > 0){
            auto results = Instrumentor::Results();
            for(int i = 0; i < results.size(); i++){ 
                if(results[i].parent >= 0) continue;
                DrawNode(i);
            }
        }
    }
    ImGui::End();
}

}