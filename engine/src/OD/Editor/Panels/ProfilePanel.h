#pragma once

#include "OD/Editor/EditorPanel.h"
#include "OD/Core/Instrumentor.h"

namespace OD{

class ProfilePanel: public EditorPanel{
public:
    ProfilePanel(){
        name = "ProfilePanel";
        show = true;
    }

    void OnGui() override{
        if(ImGui::Begin("Profile", &show)){
            for(auto i: Instrumentor::Get().results()){ 
                float durration = (i.end - i.start) * 0.001f;
                ImGui::Text("%s: %.3f.ms", i.name, durration);
            }
            Instrumentor::Get().results().clear();
        }
        ImGui::End();
    }
};

}