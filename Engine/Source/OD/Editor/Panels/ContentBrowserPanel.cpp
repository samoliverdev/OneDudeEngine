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
#include <imgui/imgui_internal.h>
#include <filesystem>
#include <string>
#include <algorithm>
#include <fstream>

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

    // Navigation controls
    if(ImGui::Button("Back") && _curDirectory != _assetsDirectory) {
        _curDirectory = _curDirectory.parent_path();
        _selectedFile.clear(); // Clear selected file when navigating
    }

    ImGui::SameLine();
    ImGui::Text("Current Directory: %s", _curDirectory.string().c_str());

    //HandleDragDrop(_curDirectory, true);

    // Right-click context menu for the panel background
    if (ImGui::BeginPopupContextWindow()) {
        HandleContextMenu(_curDirectory, true, true); // Treat as directory
        ImGui::EndPopup();
    }

    DrawDir(_curDirectory, _assetsDirectory);

    /*// Process deletions at the end of OnGui
    for (const auto& [path, isDirectory] : toDelete) {
        try {
            if (isDirectory) {
                std::filesystem::remove_all(path);
                LogInfo("Deleted directory: %s", path.string().c_str());
            } else {
                std::filesystem::remove(path);
                LogInfo("Deleted file: %s", path.string().c_str());
            }
            // Invalidate parent cache (skip if deleting _curDirectory)
            if (!isDirectory || path != _curDirectory) {
                _dirCache.erase(isDirectory ? path.parent_path() : path.parent_path());
            }
            // Clear selected file if deleted
            if (_selectedFile == path) _selectedFile.clear();
        } catch (const std::filesystem::filesystem_error& e) {
            LogError("Failed to delete %s: %s", path.string().c_str(), e.what());
        }
    }
    toDelete.clear(); // Clear after processing*/

    // Process deletions with confirmation popup
    static std::filesystem::path pendingDeletePath;
    static bool pendingDeleteIsDirectory = false;
    static bool popupActive = false;

    // If no popup is active and there are items to delete, start processing the next item
    if (!popupActive && !toDelete.empty()) {
        auto [path, isDirectory] = toDelete.front();
        toDelete.erase(toDelete.begin()); // Remove the item from the queue
        pendingDeletePath = path;
        pendingDeleteIsDirectory = isDirectory;
        ImGui::OpenPopup("Confirm Delete");
        popupActive = true;
        LogInfo("Opened delete confirmation popup for %s", path.string().c_str());
    }

    // Delete confirmation popup
    if (ImGui::BeginPopupModal("Confirm Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to delete %s?", pendingDeletePath.filename().string().c_str());
        ImGui::Separator();
        if (ImGui::Button("Yes")) {
            try {
                if (pendingDeleteIsDirectory) {
                    std::filesystem::remove_all(pendingDeletePath);
                    LogInfo("Deleted directory: %s", pendingDeletePath.string().c_str());
                } else {
                    std::filesystem::remove(pendingDeletePath);
                    LogInfo("Deleted file: %s", pendingDeletePath.string().c_str());
                }
                // Invalidate parent cache (skip if deleting _curDirectory)
                if (!pendingDeleteIsDirectory || pendingDeletePath != _curDirectory) {
                    _dirCache.erase(pendingDeleteIsDirectory ? pendingDeletePath.parent_path() : pendingDeletePath.parent_path());
                }
                // Clear selected file if deleted
                if (_selectedFile == pendingDeletePath) _selectedFile.clear();
            } catch (const std::filesystem::filesystem_error& e) {
                LogError("Failed to delete %s: %s", pendingDeletePath.string().c_str(), e.what());
            }
            ImGui::CloseCurrentPopup();
            popupActive = false; // Allow next item to be processed
            LogInfo("Confirmed deletion for %s", pendingDeletePath.string().c_str());
        }
        ImGui::SameLine();
        if (ImGui::Button("No")) {
            ImGui::CloseCurrentPopup();
            popupActive = false; // Allow next item to be processed
            LogInfo("Canceled deletion for %s", pendingDeletePath.string().c_str());
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

std::string ContentBrowserPanel::GenerateUniqueName(const std::filesystem::path& dir, const std::string& baseName, const std::string& extension) {
    std::string name = baseName + extension;
    int counter = 0;
    while (std::filesystem::exists(dir / name)) {
        name = baseName + std::to_string(++counter) + extension;
    }
    return name;
}

bool ContentBrowserPanel::CacheDirectory(const std::filesystem::path& path) {
    auto it = _dirCache.find(path);
    bool needsUpdate = true;

    // Check if cache exists and is up-to-date
    if (it != _dirCache.end() && it->second.valid) {
        try {
            auto lastWriteTime = std::filesystem::last_write_time(path);
            if (it->second.lastModified == lastWriteTime) {
                needsUpdate = false;
            }
        } catch (const std::filesystem::filesystem_error& e) {
            LogError("Failed to get last write time for %s: %s", path.string().c_str(), e.what());
            needsUpdate = true; // Force update on error
        }
    }

    if (!needsUpdate) return false;

    CachedDir cache;
    try {
        cache.lastModified = std::filesystem::last_write_time(path);
    } catch (const std::filesystem::filesystem_error& e) {
        LogError("Failed to set cache last write time for %s: %s", path.string().c_str(), e.what());
        return false;
    }

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
        LogInfo("Cached directory: %s", path.string().c_str());
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        LogError("Failed to cache directory %s: %s", path.string().c_str(), e.what());
        return false;
    }
}

void ContentBrowserPanel::HandleContextMenu(const std::filesystem::path& path, bool isDirectory, bool skipDelete) {
    LogInfo("Context menu opened for %s", path.string().c_str());
    std::filesystem::path targetDir = isDirectory ? path : path.parent_path();

    if (ImGui::MenuItem("Create File")) {
        std::filesystem::path newFilePath = targetDir / GenerateUniqueName(targetDir, "NewFile", ".txt");
        try {
            std::ofstream file(newFilePath);
            if (!file) throw std::runtime_error("Failed to open file for writing");
            file.close();
            LogInfo("Created file: %s", newFilePath.string().c_str());
            _dirCache.erase(targetDir); // Invalidate cache
        } catch (const std::exception& e) {
            LogError("Failed to create file %s: %s", newFilePath.string().c_str(), e.what());
        }
    }
    if (ImGui::MenuItem("Create Folder")) {
        std::filesystem::path newFolderPath = targetDir / GenerateUniqueName(targetDir, "NewFolder", "");
        try {
            std::filesystem::create_directory(newFolderPath);
            LogInfo("Created folder: %s", newFolderPath.string().c_str());
            _dirCache.erase(targetDir); // Invalidate cache
        } catch (const std::filesystem::filesystem_error& e) {
            LogError("Failed to create folder %s: %s", newFolderPath.string().c_str(), e.what());
        }
    }
    if (!skipDelete && ImGui::MenuItem("Delete")) {
        toDelete.emplace_back(path, isDirectory);
        LogInfo("Added to delete queue: %s", path.string().c_str());
    }
}

void ContentBrowserPanel::HandleDragDrop(const std::filesystem::path& path, bool isDirectory){
    auto getExtension = [](const std::filesystem::path& path) -> std::string {
        return path.has_extension() ? path.extension().string() : "";
    };

    if(ImGui::BeginDragDropTarget()){
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EntityMoveDragDrop");
        if(payload != nullptr){
            LogWarning("Save Prefab To: %s", path.string().c_str());
            Entity* targetEntity = (Entity*)payload->Data;

            InfoComponent& info = scene->GetComponent<InfoComponent>(*targetEntity);

            if(info.Type() == EntityType::Stand){
                if(isDirectory){
                    scene->Save((path.string() + "/" + info.name + ".prefab").c_str(), *targetEntity);
                } else if(getExtension(path) == ".prefab"){
                    scene->Save(path.string().c_str(), *targetEntity);
                }   
            } else {
                LogError("Trying create prefab from other prefab entity");
            }
        }

        ImGui::EndDragDropTarget();
    }
}

void ContentBrowserPanel::DrawDir(const std::filesystem::path& path, const std::filesystem::path& rootPath) {
    CacheDirectory(path);
    auto& cache = _dirCache[path];

    // Draw directories
    for (const auto& dir : cache.directories) {
        const auto& dirPath = dir.path();
        std::string filename = dirPath.filename().string();
        std::string label = /*ICON_FA_FOLDER +*/ std::string("##") + dirPath.string();

        ImGui::PushID(label.c_str());
        bool isSelected = (dirPath == _selectedFile);
        ImGuiTreeNodeFlags flags = (isSelected ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_SpanAvailWidth;

        //bool isOpen = ImGui::TreeNodeEx(label.c_str(), flags, "%s  %s", ICON_FA_FOLDER, filename.c_str());

        bool isOpen = ImGui::TreeNodeEx(label.c_str(), flags);

        HandleDragDrop(dirPath, true);
            
        // Context menu for both open and collapsed folders
        if (ImGui::BeginPopupContextItem()) {
            HandleContextMenu(dirPath, true);
            ImGui::EndPopup();
        }
        
        // Render colored icon
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.843f, 0.0f, 1.0f)); // Yellow for folders
        ImGui::Text("%s", ICON_FA_FOLDER);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        // Render filename in default color
        ImGui::Text("%s", filename.c_str());

        if (isOpen) {
            if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()) {
                _curDirectory = dirPath; // Navigate into directory
                _selectedFile.clear();   // Clear selected file
            } else if (ImGui::IsItemClicked()) {
                _selectedFile = dirPath; // Select directory
            }
            DrawDir(dirPath, rootPath); // Recursively draw subdirectory
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    // Draw files
    for (const auto& file : cache.files) {
        const auto& filePath = file.path();
        std::string filename = filePath.filename().string();
        std::string label = /*ICON_FA_FILE +*/ std::string("##") + filePath.string();

        ImGui::PushID(label.c_str());
        ImGuiTreeNodeFlags flags = (filePath == _selectedFile ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_Leaf;

        bool isOpen = ImGui::TreeNodeEx(label.c_str(), flags, "%s  %s", ICON_FA_FILE, filename.c_str());

        HandleDragDrop(filePath, false);
        
        /*bool isOpen = ImGui::TreeNodeEx(label.c_str(), flags);
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.678f, 0.847f, 0.902f, 1.0f)); // Light blue for files
        ImGui::Text("%s", ICON_FA_FILE);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        // Render filename in default color
        ImGui::Text("%s", filename.c_str());*/

        if (isOpen) {
            // Context menu for files
            if (ImGui::BeginPopupContextItem()) {
                HandleContextMenu(filePath, false);
                ImGui::EndPopup();
            }

            if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()) {
                _selectedFile = filePath;
                std::string pathString = _selectedFile.string();
                std::replace(pathString.begin(), pathString.end(), '\\', '/');

                auto ext = _selectedFile.extension().string();
                if (AssetTypesDB::Get().HasAssetByExtension(ext)) {
                    editor->SetSelectionAsset(
                        AssetTypesDB::Get().assetFuncs[ext].CreateFromFile(pathString));
                }
            } else if (ImGui::IsItemClicked()) {
                _selectedFile = filePath; // Select file
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