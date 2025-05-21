/*
#include "ContentBrowserPanel.h"
#include "OD/Editor/Editor.h"
#include "OD/Core/ImGui.h"
#include <filesystem>
#include <string>
#include <algorithm>

namespace OD{

std::filesystem::path _assetsDirectory;
std::filesystem::path _curDragDrop;

ContentBrowserPanel::ContentBrowserPanel(){
    name = "ContentBrowserPanel";
    show = true;

    _assetsDirectory = std::filesystem::current_path();
    _curDirectory = _assetsDirectory;

    LogInfo("ContentBrowserPanel CurDirectory: %s", _curDirectory.string().c_str());
}

void ContentBrowserPanel::OnGui(){
    _assetsDirectory = std::filesystem::current_path();
    _curDirectory = _assetsDirectory;

    ImGui::Begin("ContentBrowserPanel");
    DrawDir(_assetsDirectory, _assetsDirectory);

    ImGui::End();
}

void ContentBrowserPanel::DrawDir(std::filesystem::path path, std::filesystem::path rootPath){
    std::hash<std::string> hasher;
    
    // Draw Directorys
    for(auto& p: std::filesystem::directory_iterator(path)){
        auto& _path = p.path();
        auto relativePath = std::filesystem::relative(_path, path );
        std::string relativePathString = relativePath.string();

        if(p.is_directory() == false) continue;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
        if(ImGui::TreeNodeEx((void*)hasher(relativePathString), flags, "%s  %s", ICON_FA_FOLDER, relativePathString.c_str())){
            if(p.is_directory()) DrawDir(_path, rootPath);
            ImGui::TreePop();
        }
    }

    // Draw Files
    for(auto& p: std::filesystem::directory_iterator(path)){
        auto& _path = p.path();
        auto relativePath = std::filesystem::relative(_path, path);
        auto relativePath2 = std::filesystem::relative(_path, rootPath);
        std::string relativePathString = relativePath.string();

        if(p.is_directory() == true) continue;
        if(_path.extension() == ".meta") continue;

        ImGuiTreeNodeFlags flags = (_path == _selectedFile ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_Leaf;
        if(ImGui::TreeNodeEx((void*)hasher(relativePathString), flags, "%s  %s", ICON_FA_FILE, relativePathString.c_str())){
            if(ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered(ImGuiHoveredFlags_None)){
                _selectedFile = _path;
                std::string _pathString = _selectedFile.string();
                std::replace(_pathString.begin(), _pathString.end(), '\\', '/'); // replace all 'x' to 'y'

                if(AssetTypesDB::Get().HasAssetByExtension(_selectedFile.extension().string())){
                    editor->SetSelectionAsset(AssetTypesDB::Get().assetFuncs[_selectedFile.extension().string()].CreateFromFile(_pathString));
                }
            }

            if(ImGui::BeginDragDropSource()){
                _curDragDrop = relativePath2; //_path;
                ImGui::SetDragDropPayload(FILE_MOVE_PAYLOAD, &_curDragDrop, sizeof(_curDragDrop), ImGuiCond_Once);
                ImGui::EndDragDropSource();
            }

            if(p.is_directory()) DrawDir(_path, rootPath);
            ImGui::TreePop();
        }
    }
}

}
*/

///*
#include "ContentBrowserPanel.h"
#include "OD/Editor/Editor.h"
#include "OD/Core/ImGui.h"
#include <filesystem>
#include <string>
#include <algorithm>

namespace OD {

static std::filesystem::path _curDragDrop;

ContentBrowserPanel::ContentBrowserPanel() {
    name = "ContentBrowserPanel";
    show = true;

    _assetsDirectory = std::filesystem::current_path();
    _curDirectory = _assetsDirectory;

    LogInfo("ContentBrowserPanel CurDirectory: %s", _curDirectory.string().c_str());
}

void ContentBrowserPanel::OnGui() {
    ImGui::Begin("ContentBrowserPanel");

    // Optional: Add navigation controls to change _curDirectory
    //if (ImGui::Button("Back") && _curDirectory != _assetsDirectory) {
    //    _curDirectory = _curDirectory.parent_path();
    //    _selectedFile.clear(); // Clear selected file when navigating
    //}

    //ImGui::SameLine();
    //ImGui::Text("Current Directory: %s", _curDirectory.string().c_str());

    DrawDir(_curDirectory, _assetsDirectory);

    ImGui::End();
}

bool ContentBrowserPanel::CacheDirectory(const std::filesystem::path& path) {
    auto it = _dirCache.find(path);
    bool needsUpdate = true;

    // Check if cache exists and is up-to-date
    if (it != _dirCache.end() && it->second.valid) {
        auto lastWriteTime = std::filesystem::last_write_time(path);
        if (it->second.lastModified == lastWriteTime) {
            needsUpdate = false;
        }
    }

    if (!needsUpdate) return false;

    CachedDir cache;
    cache.lastModified = std::filesystem::last_write_time(path);

    try {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            const auto& entryPath = entry.path();
            const auto& filename = entryPath.filename().string();

            // Skip hidden files and .meta files (C++17 compatible)
            if (!filename.empty() && filename[0] == '.' || entryPath.extension() == ".meta") {
                continue;
            }

            if (entry.is_directory()) {
                cache.directories.push_back(entry);
            } else {
                cache.files.push_back(entry);
            }
        }

        // Sort directories and files for consistent display
        std::sort(cache.directories.begin(), cache.directories.end(),
                  [](const auto& a, const auto& b) {
                      return a.path().filename().string() < b.path().filename().string();
                  });
        std::sort(cache.files.begin(), cache.files.end(),
                  [](const auto& a, const auto& b) {
                      return a.path().filename().string() < b.path().filename().string();
                  });

        cache.valid = true;
        _dirCache[path] = std::move(cache);
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        LogError("Failed to cache directory %s: %s", path.string().c_str(), e.what());
        return false;
    }
}

void ContentBrowserPanel::DrawDir(const std::filesystem::path& path, const std::filesystem::path& rootPath) {
    CacheDirectory(path);
    auto& cache = _dirCache[path];

    // Draw directories
    for (const auto& dir : cache.directories) {
        const auto& dirPath = dir.path();
        std::string filename = dirPath.filename().string();
        std::string label = ICON_FA_FOLDER + std::string("##") + dirPath.string();

        ImGui::PushID(label.c_str());
        if (ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth, "%s  %s", ICON_FA_FOLDER, filename.c_str())) {
            DrawDir(dirPath, rootPath); // Recursively draw subdirectory
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    // Draw files
    for (const auto& file : cache.files) {
        const auto& filePath = file.path();
        std::string filename = filePath.filename().string();
        std::string label = ICON_FA_FILE + std::string("##") + filePath.string();

        ImGui::PushID(label.c_str());
        ImGuiTreeNodeFlags flags = (filePath == _selectedFile ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_Leaf;

        if (ImGui::TreeNodeEx(label.c_str(), flags, "%s  %s", ICON_FA_FILE, filename.c_str())) {
            if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()) {
                _selectedFile = filePath;
                std::string pathString = _selectedFile.string();
                std::replace(pathString.begin(), pathString.end(), '\\', '/');

                auto ext = _selectedFile.extension().string();
                if (AssetTypesDB::Get().HasAssetByExtension(ext)) {
                    editor->SetSelectionAsset(
                        AssetTypesDB::Get().assetFuncs[ext].CreateFromFile(pathString));
                }
            }

            // Drag and drop
            if (ImGui::BeginDragDropSource()) {
                auto relativePath = std::filesystem::relative(filePath, rootPath);
                _curDragDrop = relativePath;
                ImGui::SetDragDropPayload(FILE_MOVE_PAYLOAD, &_curDragDrop, sizeof(_curDragDrop), ImGuiCond_Once);
                ImGui::EndDragDropSource();
            }

            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}

}
//*/