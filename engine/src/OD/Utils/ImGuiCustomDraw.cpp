#include "ImGuiCustomDraw.h"
#include "OD/Core/AssetManager.h"

#include "OD/Editor/Editor.h"

namespace ImGui{

using namespace OD;

void DrawMaterialAsset(std::string& name, OD::Ref<OD::Material>& asset, OD::Ref<OD::Material> preview){
    ImGui::BeginGroup();

    if(preview != nullptr){
        //ImGui::Text("%s: %s", name, asset == nullptr ? preview->Path().c_str() : asset->Path().c_str());

        std::string field = asset == nullptr ? /*preview->Path()*/ "Default" : asset->Path();
        char _field[160];
        strcpy(_field, field.c_str()); 
        ImGui::InputText(name.c_str(), _field, field.size(), ImGuiInputTextFlags_ReadOnly);

        if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)){
            if(Editor::Get() != nullptr){
                Editor::Get()->SetSelectionAsset(asset == nullptr ? preview : asset);
            }
        }
    } else {
        //ImGui::Text("%s: %s", name, asset == nullptr ? "None" : asset->Path().c_str());

        std::string field = asset == nullptr ? "None" : asset->Path();
        char _field[160];
        strcpy(_field, field.c_str()); 
        ImGui::InputText(name.c_str(), _field, field.size(), ImGuiInputTextFlags_ReadOnly);

        if(asset != nullptr && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)){
            if(Editor::Get() != nullptr){
                Editor::Get()->SetSelectionAsset(asset);
            }
        }
    }

    if(asset != nullptr){
        ImGui::SameLine();
        if(ImGui::SmallButton("X")){
            asset = nullptr;
        }
    }
    
    ImGui::EndGroup();

    ImGui::AcceptFileMovePayload([&](std::filesystem::path* path){
        if(path->string().empty() == false && path->extension() == ".material"){
            asset = AssetManager::Get().LoadMaterial(path->string());
        }
    });
}

}