#pragma once
#include "OD/Defines.h"
#include "OD/Core/Color.h"
#include "OD/Core/Asset.h"
#include "OD/Graphics/Material.h"
#include "OD/Platform/Platform.h"
#include <imgui/imgui.h>
#include <filesystem>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <magic_enum/magic_enum.hpp>

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

            if(ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)){
                T tempT;
                std::vector<std::string> ets = tempT.GetFileAssociations();
                std::string path = OD::Platform::OpenFile(ets[0].c_str()); 
                if(path.empty() == false){
                    auto _path = std::filesystem::path(path);
                    auto relativePath2 = std::filesystem::relative(_path, std::filesystem::current_path());

                    std::string path2 = relativePath2.string();
                    std::replace(path2.begin(), path2.end(), '\\', '/'); // replace all 'x' to 'y'

                    asset = OD::AssetManager::Get().LoadAsset<T>(path2);
                    changed = true;
                }
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
            std::string _path = path->string();
            std::replace(_path.begin(), _path.end(), '\\', '/'); // replace all 'x' to 'y'

            T tempT;
            if(_path.empty() == false && tempT.HasFileExtension(path->extension().string()) == true){
                asset = OD::AssetManager::Get().LoadAsset<T>(_path);
                changed = true;
            }
        });

        return changed;
    }

    inline bool DrawPath(std::string& name, std::string& asset, std::vector<std::string>& extension){
        bool changed = false;

        ImGui::BeginGroup();

        std::string field = asset;
        char _field[160];
        strcpy(_field, field.c_str()); 
        ImGui::InputText(name.c_str(), _field, field.size(), ImGuiInputTextFlags_ReadOnly);
        
        ImGui::SameLine();
        if(ImGui::SmallButton("X")){
            asset = "";
            changed = true;
        }
        
        ImGui::EndGroup();

        ImGui::AcceptFileMovePayload([&](std::filesystem::path* path){
            std::string _path = path->string();
            std::replace(_path.begin(), _path.end(), '\\', '/'); // replace all 'x' to 'y'

            auto HasFileExtension = [&](const std::string& fileExtension){
                for(auto& i: extension){
                    if(i == fileExtension) return true;
                }
                return false;
            };

            if(_path.empty() == false && HasFileExtension(path->extension().string()) == true){
                asset = _path;
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