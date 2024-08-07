#pragma once

#include "OD/Defines.h"
#include "OD/Core/Color.h"
#include <imgui/imgui.h>
#include <filesystem>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <magic_enum/magic_enum.hpp>
#include "OD/Core/Asset.h"
#include "OD/Graphics/Material.h"

namespace ImGui{
    void OD_API _SelectionAsset(OD::Ref<OD::Asset> asset);

    void OD_API AcceptFileMovePayload(std::function<void(std::filesystem::path*)> func);
    void OD_API ColorEdit3(const char* name, OD::Color* color, ImGuiColorEditFlags flags = 0);
    void OD_API ColorEdit4(const char* name, OD::Color* color, ImGuiColorEditFlags flags = 0);

    template <class E, std::enable_if_t<std::is_enum<E>{}> * = nullptr>
    bool DrawEnumCombo(const char* name, E* e, ImGuiComboFlags flags = 0){
        static std::unique_ptr<std::vector<std::string>> enumNames{};
        if(!enumNames){
            enumNames = std::make_unique<std::vector<std::string>>();
            auto vals = magic_enum::enum_names<E>();
            for(auto &v : vals) {
                enumNames->push_back({v.begin(), v.end()});
            }
        }

        std::string currentItem = std::string(magic_enum::enum_name(*e));

        //ImGui::Text("%s", name);
        if(ImGui::BeginCombo(/*"##enum_combo"*/ name, currentItem.c_str(), flags)){
            for(int n = 0; n < enumNames->size(); n++) {
                bool is_selected = (currentItem == enumNames->at(n));
                if(ImGui::Selectable(enumNames->at(n).c_str(), is_selected)) {
                    currentItem = enumNames->at(n);
                    std::optional<E> getter = magic_enum::enum_cast<E>(currentItem);
                    if(getter.has_value()){
                        *e = getter.value();
                    }
                }
                if(is_selected){
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
            return true;
        }

        return false;
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

    template<class T>
    bool DrawAsset(std::string& name, OD::Ref<T>& asset, const OD::Ref<T> preview = nullptr){
        bool changed = false;

        ImGui::BeginGroup();

        if(preview != nullptr){
            std::string field = asset == nullptr ? "Default" : asset->Path();
            char _field[160];
            strcpy(_field, field.c_str()); 
            ImGui::InputText(name.c_str(), _field, field.size(), ImGuiInputTextFlags_ReadOnly);

            if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)){
                _SelectionAsset(asset == nullptr ? preview : asset);
            }
        } else {
            std::string field = asset == nullptr ? "None" : asset->Path();
            char _field[160];
            strcpy(_field, field.c_str()); 
            ImGui::InputText(name.c_str(), _field, field.size(), ImGuiInputTextFlags_ReadOnly);
            
            if(asset != nullptr && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)){
                _SelectionAsset(asset);
            }
        }

        if(asset != nullptr){
            ImGui::SameLine();
            if(ImGui::SmallButton("X")){
                asset = nullptr;
                changed = true;
            }
        }
        
        ImGui::EndGroup();

        ImGui::AcceptFileMovePayload([&](std::filesystem::path* path){
            T tempT;
            if(path->string().empty() == false && tempT.HasFileExtension(path->extension().string()) == true){
                asset = OD::AssetManager::Get().LoadAsset<T>(path->string());
                changed = true;
            }
        });

        return changed;
    }
}

namespace OD{

class OD_API ImGuiLayer{
public:    
    static void SetDarkTheme();
    static void SetCleanAll(bool value);
    static bool GetCleanAll();
};

}