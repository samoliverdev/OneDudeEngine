#include "ImGuiCustomDraw.h"
#include "OD/Core/AssetManager.h"

#include "OD/Editor/Editor.h"

namespace ImGui{

using namespace OD;

void DrawMaterialAsset(const char* name, OD::Ref<OD::Material>& asset){

    ImGui::BeginGroup();

    ImGui::Text("%s: %s", name, asset == nullptr ? "None" : asset->Path().c_str());

    if (asset != nullptr && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)){
        if(Editor::Get() != nullptr){
            Editor::Get()->SetSelectionAsset(asset);
        }
    }

    ImGui::SameLine();
    if(ImGui::SmallButton("X")){
        asset = nullptr;
    }
    ImGui::EndGroup();

    ImGui::AcceptFileMovePayload([&](std::filesystem::path* path){
        if(path->string().empty() == false && path->extension() == ".material"){
            asset = AssetManager::Get().LoadMaterial(path->string());
        }
    });
}

}