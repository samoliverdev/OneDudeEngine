#pragma once
#include "OD/Defines.h"
#include "OD/Core/Color.h"
#include "OD/Core/Asset.h"
#include "OD/Scene/Scene.h"
#include "OD/Graphics/Material.h"
#include "OD/Platform/Platform.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
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

    template<typename T>
    void DrawEnumCombo(const char* name, T& enumValue, const std::vector<std::string>& lookupNames, int count){
        std::string curProjectionTypeString = lookupNames[(int)enumValue];
        if(ImGui::BeginCombo(name, curProjectionTypeString.c_str())){
            for(int i = 0; i < count; i++){
                bool isSelected = curProjectionTypeString == lookupNames[i];
                if(ImGui::Selectable(lookupNames[i].c_str(), isSelected)){
                    curProjectionTypeString = lookupNames[i];
                    enumValue = (T)i;
                }
                if(isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    template<class T>
    bool DrawAsset(const std::string& name, OD::Ref<T>& asset, const OD::Ref<T> preview = nullptr){
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

    inline void DrawLayerMask(const char* label, OD::LayerMask& value){
        auto& layerNames = OD::GetGlobalSceneData().layerNames; // Retrieve global layer names
        if (layerNames.empty()) return;

        int totalLayers = layerNames.size();
        int selectedCount = 0;
        std::string selectedLayerName;

        // Count selected layers & track the first one
        for (int i = 0; i < totalLayers; i++) {
            int v = (1 << i);
            if (value.mask & v) {
                selectedCount++;
                selectedLayerName = layerNames[i]; // Store the last selected layer name
            }
        }

        // Determine what to display on the button
        std::string selectedLayersText;
        if (selectedCount == 0) {
            selectedLayersText = "Nothing";
        } else if (selectedCount == totalLayers) {
            selectedLayersText = "Everything";
        } else if (selectedCount == 1) {
            selectedLayersText = selectedLayerName;
        } else {
            selectedLayersText = "Mixed";
        }

        ImGui::TextUnformatted(label);
        ImGui::SameLine();

        // Dropdown button
        if (ImGui::Button(selectedLayersText.c_str())) {
            ImGui::OpenPopup(label);
        }

        // Dropdown popup
        if (ImGui::BeginPopup(label)) {
            bool allSelected = (selectedCount == totalLayers);
            bool noneSelected = (selectedCount == 0);

            // Shortcut checkboxes
            if (ImGui::Checkbox("Everything", &allSelected)) {
                value.mask = allSelected ? ((1 << totalLayers) - 1) : 0; // Set all or none
                //ImGui::CloseCurrentPopup();
            }
            if (ImGui::Checkbox("Nothing", &noneSelected)) {
                value.mask = 0; // Clear all
                //ImGui::CloseCurrentPopup();
            }

            // Layer checkboxes (disabled if Everything/Nothing was selected)
            bool disableOtherChecks = allSelected || noneSelected;
            for (int i = 0; i < totalLayers; i++) {
                int v = (1 << i);
                bool selected = (value.mask & v) != 0;
                if (ImGui::Checkbox(layerNames[i].c_str(), &selected)){
                    if (selected)
                        value.mask |= v;  // Add layer
                    else
                        value.mask &= ~v; // Remove layer
                }
            }

            ImGui::EndPopup();
        }
    }

    constexpr ImGuiID GlobalTableID = (ImGuiID)23443434;

    /*inline void BeginGlobalTable(const char* name){
        ImGui::BeginTableEx(
            name, ImGui::GlobalTableID, 2,
            ImGuiTableFlags_NoSavedSettings |
            ImGuiTableFlags_Resizable |                    // Permite redimensionar colunas manualmente
            //ImGuiTableFlags_SizingStretchSame |            // Faz com que as colunas preencham o espaço igualmente
            ImGuiTableFlags_SizingStretchProp |
            ImGuiTableFlags_NoPadOuterX | 
            ImGuiTableFlags_BordersInnerV
        );
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    }

    inline void EndGlobalTable(){
        ImGui::EndTable();
    }

    template<typename Func>
    void GlobalTableRow(const char* label, const char* id, Func&& renderControl) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text(label);
        ImGui::TableSetColumnIndex(1);
        ImGui::PushItemWidth(-1);
        renderControl(id);
        ImGui::PopItemWidth();
    }

    template<typename ImGuiFunc, typename... Args>
    void GlobalTableRow2(const char* label, const char* id, ImGuiFunc imguiFunc, Args&&... args) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(label);
        ImGui::TableSetColumnIndex(1);
        ImGui::PushItemWidth(-1);
        imguiFunc(id, std::forward<Args>(args)...);
        ImGui::PopItemWidth();
    }

    template<typename Func, typename... Args>
    void GlobalTableRow3(const char* label, Func imguiFunc, Args&&... args) {
        char id[64];
        snprintf(id, sizeof(id), "##%s", label);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(label);
        ImGui::TableSetColumnIndex(1);
        ImGui::PushItemWidth(-1);

        imguiFunc(id, std::forward<Args>(args)...);  // Call the ImGui function

        ImGui::PopItemWidth();
    }*/
}

#define IMGUI_BeginGlobalTable(name) \
    ImGui::BeginTableEx(name, ImGui::GlobalTableID, 2,ImGuiTableFlags_NoSavedSettings |ImGuiTableFlags_Resizable |ImGuiTableFlags_SizingStretchProp |ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_BordersInnerV); \
    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch, 1.0f); \
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 1.0f);

#define IMGUI_EndGlobalTable() \
    ImGui::EndTable();

#define IMGUI_GlobalTableRow(label, func) \
    ImGui::TableNextRow();             \
    ImGui::TableSetColumnIndex(0);     \
    ImGui::TextUnformatted(label);     \
    ImGui::TableSetColumnIndex(1);     \
    ImGui::PushItemWidth(-1);          \
    func;                              \
    ImGui::PopItemWidth();

//#define OD_GlobalTableRow(label, func, code) ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(label); ImGui::TableSetColumnIndex(1); ImGui::PushItemWidth(-1); if(func) { code } ImGui::PopItemWidth();


namespace OD{

class OD_API ImGuiLayer{
public:    
    static void SetDarkTheme();
    static void SetCleanAll(bool value);
    static bool GetCleanAll();
};

}