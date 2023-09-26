#include "ContentBrowserPanel.h"
#include "OD/Core/ImGui.h"
#include <filesystem>
#include <string>

namespace OD{

std::filesystem::path _assetsDirectory = "res/";

ContentBrowserPanel::ContentBrowserPanel(){
    _curDirectory = _assetsDirectory;
}

void ContentBrowserPanel::OnGui(){
    ImGui::Begin("ContentBrowserPanel");
    DrawDir(_assetsDirectory, _assetsDirectory);

    if(ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()){
        _selectedFile = std::filesystem::path();
    }

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

        ImGuiTreeNodeFlags flags = (_path == _selectedFile ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_Leaf;
        if(ImGui::TreeNodeEx((void*)hasher(relativePathString), flags, relativePathString.c_str())){
            if(ImGui::IsItemClicked()){
                _selectedFile = _path;
            }

            if(_path == _selectedFile && ImGui::BeginDragDropSource()){
                ImGui::SetDragDropPayload("ContentBrowserPanelFile", &_selectedFile, sizeof(_selectedFile), ImGuiCond_Once);
                ImGui::EndDragDropSource();
            }

            if(p.is_directory()) DrawDir(_path, rootPath);
            ImGui::TreePop();
        }
    }
}


}