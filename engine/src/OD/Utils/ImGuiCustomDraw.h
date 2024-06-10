#pragma once

#include "OD/Core/ImGui.h"
#include "OD/Graphics/Material.h"
#include "OD/Core/AssetManager.h"

namespace ImGui{
    void DrawMaterialAsset(std::string& name, OD::Ref<OD::Material>& asset, OD::Ref<OD::Material> preview = nullptr);

    template<class T>
    void DrawAsset(std::string& name, OD::Ref<T>& asset, OD::Ref<T> preview = nullptr){
        ImGui::BeginGroup();

        if(preview != nullptr){
            std::string field = asset == nullptr ? "Default" : asset->Path();
            char _field[160];
            strcpy(_field, field.c_str()); 
            ImGui::InputText(name.c_str(), _field, field.size(), ImGuiInputTextFlags_ReadOnly);

        } else {
            std::string field = asset == nullptr ? "None" : asset->Path();
            char _field[160];
            strcpy(_field, field.c_str()); 
            ImGui::InputText(name.c_str(), _field, field.size(), ImGuiInputTextFlags_ReadOnly);
        }

        if(asset != nullptr){
            ImGui::SameLine();
            if(ImGui::SmallButton("X")){
                asset = nullptr;
            }
        }
        
        ImGui::EndGroup();

        ImGui::AcceptFileMovePayload([&](std::filesystem::path* path){
            T tempT;
            if(path->string().empty() == false && tempT.HasFileExtension(path->extension().string()) == true){
                asset = OD::AssetManager::Get().LoadAsset<T>(path->string());
            }
        });
    }

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