#include "ContentBrowserPanel.h"
#include "OD/Editor/Editor.h"
#include "OD/Core/ImGui.h"
#include <filesystem>
#include <string>
#include <algorithm>

namespace OD{

std::filesystem::path _assetsDirectory = "res/";
std::filesystem::path _curDragDrop;

ContentBrowserPanel::ContentBrowserPanel(){
    _curDirectory = _assetsDirectory;
}

void ContentBrowserPanel::OnGui(){
    ImGui::Begin("ContentBrowserPanel");
    DrawDir(_assetsDirectory, _assetsDirectory);

    /*if(ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()){
        _selectedFile = std::filesystem::path();
        _editor->_selectionAsset = nullptr;
        _editor->_selectionOnAsset = false;
    }*/

    ImGui::End();
}

void ContentBrowserPanel::DrawDir(std::filesystem::path path, std::filesystem::path rootPath){
    std::hash<std::string> hasher;
    
    for(auto& p: std::filesystem::directory_iterator(path)){
        auto& _path = p.path();
        auto relativePath = std::filesystem::relative(_path, rootPath);
        std::string relativePathString = relativePath.string();

        if(p.is_directory() == false) continue;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
        if(ImGui::TreeNodeEx((void*)hasher(relativePathString), flags, relativePathString.c_str())){
            if(p.is_directory()) DrawDir(_path, rootPath);
            ImGui::TreePop();
        }
    }

    for(auto& p: std::filesystem::directory_iterator(path)){
        auto& _path = p.path();
        auto relativePath = std::filesystem::relative(_path, rootPath);
        std::string relativePathString = relativePath.string();

        if(p.is_directory() == true) continue;
        if(_path.extension() == ".meta") continue;

        ImGuiTreeNodeFlags flags = (_path == _selectedFile ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_Leaf;
        if(ImGui::TreeNodeEx((void*)hasher(relativePathString), flags, relativePathString.c_str())){
            if(ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered(ImGuiHoveredFlags_None)){
                _selectedFile = _path;
                std::string _pathString = _selectedFile.string();
                std::replace(_pathString.begin(), _pathString.end(), '\\', '/'); // replace all 'x' to 'y'

                if(AssetTypesDB::Get().HasAssetByExtension(_selectedFile.extension().string())){
                    _editor->SetSelectionAsset(AssetTypesDB::Get()._assetFuncs[_selectedFile.extension().string()].CreateFromFile(_pathString));
                }
            }

            if(ImGui::BeginDragDropSource()){
                _curDragDrop = _path;
                ImGui::SetDragDropPayload(FILE_MOVE_PAYLOAD, &_curDragDrop, sizeof(_curDragDrop), ImGuiCond_Once);
                ImGui::EndDragDropSource();
            }

            if(p.is_directory()) DrawDir(_path, rootPath);
            ImGui::TreePop();
        }
    }
}


}