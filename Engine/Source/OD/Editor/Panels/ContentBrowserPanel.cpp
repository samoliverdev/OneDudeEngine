#include "OD/pch.h"
#include "ContentBrowserPanel.h"
#include "OD/Editor/Editor.h"
#include "OD/Core/ImGui.h"
#include "OD/Graphics/Material.h"
#include "OD/Graphics/Shader.h"
#include <imgui/imgui_internal.h>

namespace OD {

static std::filesystem::path _curDragDrop;

ContentBrowserPanel::ContentBrowserPanel() {
    name = "ContentBrowserPanel";
    show = true;

    _assetsDirectory = std::filesystem::current_path();
    _curDirectory = _assetsDirectory;

    LogInfo("ContentBrowserPanel CurDirectory: {}", _curDirectory.string());

    UpdateFileCache();
}

void ContentBrowserPanel::GoTo(const std::string& path) {
    if (show == false) return;

    std::filesystem::path targetPath = path;
    if (!targetPath.is_absolute()) {
        targetPath = _assetsDirectory / path;
    }
    try {
        targetPath = std::filesystem::canonical(targetPath).lexically_normal();
    } catch (const std::exception& e) {
        //LogError("GoTo: Failed to canonicalize path %s: {}", path.c_str(), e.what());
        return;
    }

    if (!std::filesystem::exists(targetPath)) {
        //LogError("GoTo: Path does not exist: %s", path.c_str());
        return;
    }

    _selectedFile = targetPath;
    _goToPath = targetPath;

    //LogInfo("GoTo: Selected %s, goToPath %s", _selectedFile.string().c_str(), _goToPath.string().c_str());
}


void ContentBrowserPanel::OnGui() {
    ImGui::Begin("ContentBrowserPanel");

    static char searchBuffer[128] = "";
    static std::filesystem::path lastDirectory;
    if (_curDirectory != lastDirectory) {
        searchBuffer[0] = '\0';
        searchQuery.clear();
        extensionFilter.clear();
        _filteredFiles.clear();
        lastDirectory = _curDirectory;
    }

    ImGui::Text("Search:");
    ImGui::SameLine();
    bool searchChanged = ImGui::InputText("##SearchFiles", searchBuffer, sizeof(searchBuffer));
    if (searchChanged) {
        std::string newQuery = std::string(searchBuffer);
        std::transform(newQuery.begin(), newQuery.end(), newQuery.begin(), ::tolower);
        if (newQuery != searchQuery) {
            searchQuery = newQuery;
            UpdateFilteredFiles();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("X")) {
        searchBuffer[0] = '\0';
        searchQuery.clear();
        extensionFilter.clear();
        _filteredFiles.clear();
    }

    ImGui::SameLine();
    ImGui::Text("Filter:");
    ImGui::SameLine();
    static const char* extensions[] = { "All", ".obj", ".png", ".txt", ".material", ".prefab" };
    static int currentExtension;
    if (ImGui::Combo("##ExtensionFilter", &currentExtension, extensions, IM_ARRAYSIZE(extensions))) {
        extensionFilter = (currentExtension == 0) ? "" : extensions[currentExtension];
        UpdateFilteredFiles();
    }

    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        _dirCache.clear();
        UpdateFileCache();
        UpdateFilteredFiles();
    }

    ImGui::Separator();

    if (ImGui::Button("Back") && _curDirectory != _assetsDirectory) {
        _curDirectory = _curDirectory.parent_path();
        _selectedFile.clear();
    }

    ImGui::SameLine();
    ImGui::Text("Current Directory: %s", _curDirectory.string().c_str());

    contextMenuOpen = false; // Shared across OnGui, DrawDir, DrawMatchingFiles
    if (!searchQuery.empty() || !extensionFilter.empty()) {
        DrawMatchingFiles(_assetsDirectory);
    } else {
        DrawDir(_curDirectory, _assetsDirectory);
    }

    // Background context menu only if no other menu is open
    if (!contextMenuOpen && ImGui::BeginPopupContextWindow("BackgroundContext")) {
        contextMenuOpen = true;
        LogInfo("Opening background context menu for {}", _curDirectory.string());
        HandleContextMenu(_curDirectory, true, true);
        ImGui::EndPopup();
    }

    static std::filesystem::path pendingDeletePath;
    static bool pendingDeleteIsDirectory = false;
    static bool popupActive = false;

    if (!popupActive && !toDelete.empty()) {
        auto [path, isDirectory] = toDelete.front();
        toDelete.erase(toDelete.begin());
        pendingDeletePath = path;
        pendingDeleteIsDirectory = isDirectory;
        ImGui::OpenPopup("Confirm Delete");
        popupActive = true;
        LogInfo("Opened delete confirmation popup for {}", path.string());
    }

    if (ImGui::BeginPopupModal("Confirm Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to delete %s?", pendingDeletePath.filename().string().c_str());
        ImGui::Separator();
        if (ImGui::Button("Yes")) {
            try {
                // Check if deleting affects current directory or its contents
                bool affectsCurrentDir = pendingDeleteIsDirectory &&
                    (pendingDeletePath == _curDirectory ||
                     std::filesystem::relative(_curDirectory, pendingDeletePath).string().find("..") == std::string::npos);

                // Perform deletion
                if (pendingDeleteIsDirectory) {
                    std::filesystem::remove_all(pendingDeletePath);
                    LogInfo("Deleted directory: {}", pendingDeletePath.string());
                } else {
                    std::filesystem::remove(pendingDeletePath);
                    LogInfo("Deleted file: {}", pendingDeletePath.string());
                }

                // Update current directory if needed
                if (affectsCurrentDir) {
                    _curDirectory = _assetsDirectory;
                    LogInfo("Reset _curDirectory to {} after deletion", _curDirectory.string());
                }

                // Update caches: avoid processing deleted directory
                _dirCache.erase(pendingDeletePath);
                _dirCache.erase(pendingDeletePath.parent_path());
                if (affectsCurrentDir || pendingDeleteIsDirectory) {
                    _fileCache.clear();
                    _filteredFiles.clear();
                    UpdateFileCache(); // Full rebuild
                    LogInfo("Rebuilt full cache after deleting {}", pendingDeletePath.string());
                } else {
                    UpdateFileCache(pendingDeletePath.parent_path());
                    UpdateFilteredFiles();
                    LogInfo("Updated cache for parent {}", pendingDeletePath.parent_path().string());
                }

                if (_selectedFile == pendingDeletePath) _selectedFile.clear();
            } catch (const std::exception& e) {
                LogError("Error deleting {}: {}", pendingDeletePath.string(), e.what());
            }
            ImGui::CloseCurrentPopup();
            popupActive = false;
            LogInfo("Confirmed deletion for {}", pendingDeletePath.string());
        }
        ImGui::SameLine();
        if (ImGui::Button("No")) {
            ImGui::CloseCurrentPopup();
            popupActive = false;
            LogInfo("Canceled deletion for {}", pendingDeletePath.string());
        }
        ImGui::EndPopup();
    }

    contextMenuOpen = false; // Reset for next frame
    _goToPath.clear();

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
    // Skip if path doesn't exist
    if (!std::filesystem::exists(path)) {
        LogWarning("Skipping cache for non-existent path: {}", path.string());
        return false;
    }

    auto it = _dirCache.find(path);
    bool needsUpdate = true;

    if (it != _dirCache.end() && it->second.valid) {
        try {
            auto lastWriteTime = std::filesystem::last_write_time(path);
            if (it->second.lastModified == lastWriteTime) {
                needsUpdate = false;
            }
        } catch (const std::exception& e) {
            LogError("Failed to get last write time for {}: {}", path.string(), e.what());
            needsUpdate = true;
        }
    }

    if (!needsUpdate) return false;

    CachedDir cache;
    try {
        cache.lastModified = std::filesystem::last_write_time(path);
    } catch (const std::exception& e) {
        LogError("Failed to set cache last write time for {}: {}", path.string(), e.what());
        return false;
    }

    try {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            const auto& entryPath = entry.path();
            const auto& filename = entryPath.filename().string();

            if (!filename.empty() && filename[0] == '.' || entryPath.extension() == ".meta") {
                continue;
            }

            if (entry.is_directory()) {
                cache.directories.push_back(entry);
            } else {
                cache.files.push_back(entry);
            }
        }

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
        //LogInfo("Cached directory: {}", path.string().c_str());
        return true;
    } catch (const std::exception& e){
        LogError("Failed to cache directory {}: {}", path.string(), e.what());
        return false;
    }
}

void ContentBrowserPanel::HandleContextMenu(const std::filesystem::path& path, bool isDirectory, bool skipDelete) {
    static int frameCount = ImGui::GetFrameCount();
    if (frameCount != ImGui::GetFrameCount()) {
        frameCount = ImGui::GetFrameCount();
        LogInfo(
            "Context menu opened for {} (isDirectory: {}, skipDelete: {}, frame: {})",
            path.string(), isDirectory, skipDelete, frameCount
        );
    }

    std::filesystem::path targetDir = isDirectory ? path : path.parent_path();

    if (ImGui::MenuItem("Create File")) {
        std::filesystem::path newFilePath = targetDir / GenerateUniqueName(targetDir, "NewFile", ".txt");
        try {
            std::ofstream file(newFilePath);
            if (!file) throw std::runtime_error("Failed to open file for writing");
            file.close();
            LogInfo("Created file: {}", newFilePath.string());
            _dirCache.erase(targetDir);
            UpdateFileCache(targetDir);
            UpdateFilteredFiles();
        } catch (const std::exception& e) {
            LogError("Failed to create file {}: {}", newFilePath.string(), e.what());
        }
    }
    if (ImGui::MenuItem("Create Material")) {
        std::string path = Platform::SaveFile("*.material");
        if (!path.empty()) {
            Ref<Material> mat = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit2.glsl"));
            mat->Save(path);
            UpdateFileCache(std::filesystem::path(path).parent_path());
            UpdateFilteredFiles();
        }
    }
    if (ImGui::MenuItem("Create Folder")) {
        std::filesystem::path newFolderPath = targetDir / GenerateUniqueName(targetDir, "NewFolder", "");
        try {
            std::filesystem::create_directory(newFolderPath);
            LogInfo("Created folder: {}", newFolderPath.string());
            _dirCache.erase(targetDir);
            UpdateFileCache(targetDir);
            UpdateFilteredFiles();
        } catch (const std::filesystem::filesystem_error& e) {
            LogError("Failed to create folder {}: {}", newFolderPath.string(), e.what());
        }
    }
    if (!skipDelete && ImGui::MenuItem("Delete")) {
        toDelete.emplace_back(path, isDirectory);
        LogInfo("Added to delete queue: {}", path.string());
    }
}

void ContentBrowserPanel::HandleDragDrop(const std::filesystem::path& path, bool isDirectory) {
    auto getExtension = [](const std::filesystem::path& path) -> std::string {
        return path.has_extension() ? path.extension().string() : "";
    };

    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EntityMoveDragDrop");
        if (payload != nullptr) {
            auto relativePath = std::filesystem::relative(path, _assetsDirectory);
            std::string pathString = relativePath.generic_string();
            //std::replace(pathString.begin(), pathString.end(), '\\', '/');

            LogWarning("Save Prefab To: {}", pathString);
            Entity* targetEntity = (Entity*)payload->Data;

            InfoComponent& info = scene->GetComponent<InfoComponent>(*targetEntity);

            if (info.Type() == EntityType::Stand) {
                std::filesystem::path updatePath;
                if (isDirectory) {
                    pathString = pathString + "/" + info.name + ".prefab";
                    scene->Save(pathString.c_str(), *targetEntity);
                    updatePath = path;
                } else if (getExtension(relativePath) == ".prefab") {
                    scene->Save(pathString.c_str(), *targetEntity);
                    updatePath = path.parent_path();
                }
                UpdateFileCache(updatePath);
                UpdateFilteredFiles();
            } else {
                LogError("Trying create prefab from other prefab entity");
            }
        }

        ImGui::EndDragDropTarget();
    }
}

void ContentBrowserPanel::UpdateFileCache(const std::filesystem::path& path) {
    if (path.empty()) {
        _fileCache.clear();
        try {
            CollectAllFiles(_assetsDirectory, _fileCache);
            LogInfo("Full file cache updated with {} files", _fileCache.size());
        } catch (const std::exception& e) {
            LogError("Failed to update full file cache: {}", e.what());
        }
        return;
    }

    // Skip if path doesn't exist
    if (!std::filesystem::exists(path)) {
        LogWarning("Skipping cache update for non-existent path: {}", path.string());
        return;
    }

    try {
        CacheDirectory(path);
        _fileCache.erase(
            std::remove_if(_fileCache.begin(), _fileCache.end(),
                [&path](const auto& file) { return file.entry.path().parent_path() == path; }),
            _fileCache.end());
        std::vector<FileEntry> newFiles;
        CollectAllFiles(path, newFiles);
        _fileCache.insert(_fileCache.end(), newFiles.begin(), newFiles.end());
        LogInfo("Incremental file cache updated for {} with {} total files", path.string(), _fileCache.size());
    } catch (const std::exception& e) {
        LogError("Failed to update file cache for {}: {}", path.string(), e.what());
    }
}

void ContentBrowserPanel::CollectAllFiles(const std::filesystem::path& path, std::vector<FileEntry>& outFiles) {
    // Skip if path doesn't exist
    if (!std::filesystem::exists(path)) {
        LogWarning("Skipping collect files for non-existent path: {}", path.string());
        return;
    }

    try {
        CacheDirectory(path);
        auto& cache = _dirCache[path];

        for (const auto& file : cache.files) {
            FileEntry entry;
            entry.entry = file;
            entry.filenameLower = file.path().filename().string();
            std::transform(entry.filenameLower.begin(), entry.filenameLower.end(), entry.filenameLower.begin(), ::tolower);
            outFiles.push_back(entry);
        }

        for (const auto& dir : cache.directories) {
            CollectAllFiles(dir.path(), outFiles);
        }
    } catch (const std::exception& e) {
        LogError("Failed to collect files for {}: {}", path.string(), e.what());
    }
}

void ContentBrowserPanel::UpdateFilteredFiles() {
    auto start = std::chrono::high_resolution_clock::now();
    _filteredFiles.clear();
    if (searchQuery.empty() && extensionFilter.empty()) return;

    for (const auto& file : _fileCache) {
        bool matchesQuery = searchQuery.empty() || file.filenameLower.find(searchQuery) != std::string::npos;
        bool matchesExtension = extensionFilter.empty() ||
                                file.entry.path().extension().string() == extensionFilter;
        if (matchesQuery && matchesExtension) {
            _filteredFiles.push_back(file);
        }
    }

    std::sort(_filteredFiles.begin(), _filteredFiles.end(),
              [](const auto& a, const auto& b) {
                  return a.entry.path().filename().string() < b.entry.path().filename().string();
              });
    auto end = std::chrono::high_resolution_clock::now();
    LogInfo(
        "Filtered {} files for query '{}' and extension '{}' in {} ms",
        _filteredFiles.size(), searchQuery, extensionFilter,
        std::chrono::duration<double, std::milli>(end - start).count()
    );
}

void ContentBrowserPanel::DrawMatchingFiles(const std::filesystem::path& rootPath) {
    ImGui::BeginChild("SearchResults", ImVec2(0, 0), true);

    //extern bool contextMenuOpen; // Declared in OnGui
    for (const auto& file : _filteredFiles) {
        const auto& filePath = file.entry.path();
        std::string filename = filePath.filename().string();
        std::string label = std::string("##") + filePath.string();

        ImGui::PushID(label.c_str());
        ImGuiTreeNodeFlags flags = (filePath == _selectedFile ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_Leaf;

        //bool isOpen = ImGui::TreeNodeEx(label.c_str(), flags);
        bool isOpen = ImGui::TreeNodeEx(label.c_str(), flags, "%s  %s", ICON_FA_FILE, filename.c_str());

        HandleDragDrop(filePath, false);

        /*ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.678f, 0.847f, 0.902f, 1.0f));
        ImGui::Text("%s", ICON_FA_FILE);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::Text("%s", filename.c_str());*/

        if (!contextMenuOpen && ImGui::BeginPopupContextItem(label.c_str())) {
            contextMenuOpen = true;
            LogInfo("Opening file context menu for {}", filePath.string().c_str());
            HandleContextMenu(filePath, false);
            ImGui::EndPopup();
        }

        if (isOpen) {
            if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()) {
                _selectedFile = std::filesystem::relative(filePath, rootPath);
                std::string pathString = _selectedFile.string();
                std::replace(pathString.begin(), pathString.end(), '\\', '/');

                auto ext = _selectedFile.extension().string();
                if (AssetTypesDB::Get().HasAssetByExtension(ext)) {
                    editor->SetSelectionAsset(
                        AssetTypesDB::Get().assetFuncs[ext].CreateFromFile(pathString)
                    );
                }
            } else if (ImGui::IsItemClicked()) {
                _selectedFile = filePath;
            }

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

    ImGui::EndChild();
}

void ContentBrowserPanel::DrawDir(const std::filesystem::path& path, const std::filesystem::path& rootPath) {
    CacheDirectory(path);
    auto& cache = _dirCache[path];

    auto isParentOfGoTo = [this](const std::filesystem::path& dirPath) -> bool {
        if (_goToPath.empty()) {
            //LogInfo("GoTo empty for %s", dirPath.c_str());
            return false;
        }
        try {
            auto rel = std::filesystem::relative(_goToPath, dirPath);
            bool isParent = !rel.empty() && rel.string().find("..") == std::string::npos;
            //LogInfo("GoTo check: %s, rel=%s, parent=%d", dirPath.c_str(), rel.c_str(), isParent);
            return isParent;
        } catch (const std::exception& e) {
            //LogError("GoTo error: %s: {}", dirPath.c_str(), e.what());
            return false;
        }
    };

    //extern bool contextMenuOpen; // Declared in OnGui
    for (const auto& dir : cache.directories) {
        const auto& dirPath = dir.path();
        std::string filename = dirPath.filename().string();
        std::string label = std::string("##") + dirPath.string();

        ImGui::PushID(label.c_str());
        bool isSelected = (dirPath == _selectedFile);
        ImGuiTreeNodeFlags flags = (isSelected ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_SpanAvailWidth;

        if (isParentOfGoTo(dirPath)) {
            ImGui::SetNextItemOpen(true, ImGuiCond_Always);
            //LogInfo("Expanding: %s", dirPath.c_str());
        }

        bool isOpen = ImGui::TreeNodeEx(label.c_str(), flags);

        HandleDragDrop(dirPath, true);

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.843f, 0.0f, 1.0f));
        ImGui::Text("%s", ICON_FA_FOLDER);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::Text("%s", filename.c_str());

        if (!contextMenuOpen && ImGui::BeginPopupContextItem(label.c_str())) {
            contextMenuOpen = true;
            LogInfo("Opening directory context menu for {}", dirPath.string());
            HandleContextMenu(dirPath, true);
            ImGui::EndPopup();
        }

        if (isOpen) {
            if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()) {
                _curDirectory = dirPath;
                _selectedFile.clear();
            } else if (ImGui::IsItemClicked()) {
                _selectedFile = dirPath;
            }
            DrawDir(dirPath, rootPath);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    for (const auto& file : cache.files) {
        const auto& filePath = file.path();
        std::string filename = filePath.filename().string();
        std::string label = std::string("##") + filePath.string();

        ImGui::PushID(label.c_str());
        ImGuiTreeNodeFlags flags = (filePath == _selectedFile ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_Leaf;

        //bool isOpen = ImGui::TreeNodeEx(label.c_str(), flags);
        bool isOpen = ImGui::TreeNodeEx(label.c_str(), flags, "%s  %s", ICON_FA_FILE, filename.c_str());

        if(_goToPath == filePath) {
            ImGui::SetScrollHereY();  // or SetScrollFromPosY(ImGui::GetCursorPosY(), 0.5f)
        }

        HandleDragDrop(filePath, false);

        /*ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.678f, 0.847f, 0.902f, 1.0f));
        ImGui::Text("%s", ICON_FA_FILE);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::Text("%s", filename.c_str());*/

        if (!contextMenuOpen && ImGui::BeginPopupContextItem(label.c_str())) {
            contextMenuOpen = true;
            LogInfo("Opening file context menu for {}", filePath.string());
            HandleContextMenu(filePath, false);
            ImGui::EndPopup();
        }

        if (isOpen) {
            if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()) {
                auto relativePath = std::filesystem::relative(filePath, rootPath);
                _selectedFile = relativePath;
                std::string pathString = _selectedFile.string();
                std::replace(pathString.begin(), pathString.end(), '\\', '/');

                auto ext = _selectedFile.extension().string();
                if (AssetTypesDB::Get().HasAssetByExtension(ext)) {
                    editor->SetSelectionAsset(
                        AssetTypesDB::Get().assetFuncs[ext].CreateFromFile(pathString)
                    );
                }
            } else if (ImGui::IsItemClicked()) {
                _selectedFile = filePath;
            }

            if(ImGui::BeginDragDropSource()){
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