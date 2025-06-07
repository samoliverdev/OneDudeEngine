#pragma once
#include "OD/Defines.h"
#include "OD/Editor/EditorPanel.h"
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <string>

namespace OD {

class Editor;

class OD_API ContentBrowserPanel : public EditorPanel {
public:
    ContentBrowserPanel();
    void OnGui() override;

    void GoTo(const std::string& path);

private:
    struct FileEntry {
        std::filesystem::directory_entry entry;
        std::string filenameLower; // Preprocessed lowercase filename
    };

    std::filesystem::path _curDirectory;
    std::filesystem::path _selectedFile;
    std::filesystem::path _assetsDirectory;
    std::filesystem::path _goToPath;

    std::string searchQuery; // Class member for search query
    std::string extensionFilter; // Selected extension filter (e.g., ".obj")
    std::vector<FileEntry> _fileCache; // Cache for all files
    std::vector<FileEntry> _filteredFiles; // Cache for filtered matching files
    bool contextMenuOpen = false;

    struct CachedDir {
        std::vector<std::filesystem::directory_entry> directories;
        std::vector<std::filesystem::directory_entry> files;
        std::filesystem::file_time_type lastModified;
        bool valid = false;
    };

    std::unordered_map<std::filesystem::path, CachedDir, std::hash<std::filesystem::path>> _dirCache;
    std::vector<std::pair<std::filesystem::path, bool>> toDelete; // Path and isDirectory flag

    void DrawDir(const std::filesystem::path& path, const std::filesystem::path& rootPath);
    bool CacheDirectory(const std::filesystem::path& path);
    std::string GenerateUniqueName(const std::filesystem::path& dir, const std::string& baseName, const std::string& extension);
    void HandleContextMenu(const std::filesystem::path& path, bool isDirectory, bool skipDelete = false);
    void HandleDragDrop(const std::filesystem::path& path, bool isDirectory);
    void UpdateFileCache(const std::filesystem::path& path = "");
    void CollectAllFiles(const std::filesystem::path& path, std::vector<FileEntry>& outFiles);
    void UpdateFilteredFiles();
    void DrawMatchingFiles(const std::filesystem::path& rootPath);
};

}