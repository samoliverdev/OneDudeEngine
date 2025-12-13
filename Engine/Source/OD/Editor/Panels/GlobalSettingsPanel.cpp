#include "OD/pch.h"
#include "GlobalSettingsPanel.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/GlobalSettings.h"

namespace OD{

GlobalSettingsPanel::GlobalSettingsPanel(){
    name = "GlobalSettingsPanel";
    show = true;
}

void GlobalSettingsPanel::OnGui(){
    if(show == false) return; 

    /*if(ImGui::Begin("GlobalSettingsPanel", &show)){
        
    }
    ImGui::End();*/

    OD::GlobalSettings::Get().OnImGuiRender();
}

}