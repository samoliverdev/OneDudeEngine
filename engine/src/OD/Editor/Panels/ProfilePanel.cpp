#include "ProfilePanel.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Core/ImGui.h"

namespace OD{

ProfilePanel::ProfilePanel(){
    name = "ProfilePanel";
    show = true;
}

void ProfilePanel::OnGui(){
    if(ImGui::Begin("Profile", &show)){
        ImGui::Text("Profile Count:: %zd", Instrumentor::Get().Results().size());
        for(auto i: Instrumentor::Get().Results()){ 
            float durration = (i.end - i.start) * 0.001f;
            ImGui::Text("%s: %.3f.ms", i.name, durration);
        }
        Instrumentor::Get().Results().clear();
    }
    ImGui::End();
}

}