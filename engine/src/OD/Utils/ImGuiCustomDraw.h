#pragma once

#include "OD/Core/ImGui.h"
#include "OD/Renderer/Material.h"

namespace ImGui{
    void DrawMaterialAsset(const char* name, OD::Ref<OD::Material>& asset);

    template<typename T>
    void DrawEnumCombo(const char* name, T& enumValue, const char** lookupNames, int count){
        const char* curProjectionTypeString = lookupNames[(int)enumValue];
        
        if(ImGui::BeginCombo(name, curProjectionTypeString)){
            for(int i = 0; i < count; i++){
                bool isSelected = curProjectionTypeString == lookupNames[i];
                if(ImGui::Selectable(lookupNames[i], isSelected)){
                    curProjectionTypeString = lookupNames[i];
                    enumValue = (T)i;
                }

                if(isSelected) ImGui::SetItemDefaultFocus();
                
            }

            ImGui::EndCombo();
        }
    }
}