#include "ProfilePanel.h"

namespace OD{

ProfilePanel::ProfilePanel(){
    name = "ProfilePanel";
    show = true;
}

void ProfilePanel::OnGui(){
    if(ImGui::Begin("Profile", &show)){
        ImGui::Text("Profile Count:: %zd", Instrumentor::Get().results().size());
        for(auto i: Instrumentor::Get().results()){ 
            float durration = (i.end - i.start) * 0.001f;
            ImGui::Text("%s: %.3f.ms", i.name, durration);
        }
        Instrumentor::Get().results().clear();
    }
    ImGui::End();
}

}