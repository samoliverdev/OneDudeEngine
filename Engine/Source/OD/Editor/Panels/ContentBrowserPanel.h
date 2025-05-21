/*
#pragma once
#include "OD/Defines.h"
#include "OD/Editor/EditorPanel.h"
#include <filesystem>

namespace OD{

class Editor;

class OD_API ContentBrowserPanel: public EditorPanel{
public:
    ContentBrowserPanel();
    void OnGui() override;

private:
    std::filesystem::path _curDirectory;
    std::filesystem::path _selectedFile;

    void DrawDir(std::filesystem::path path, std::filesystem::path rootPath);
};

}
*/

///*
#pragma once
#include "OD/Defines.h"
#include "OD/Editor/EditorPanel.h"
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <chrono>

namespace OD {

class Editor;

class OD_API ContentBrowserPanel : public EditorPanel {
public:
    ContentBrowserPanel();
    void OnGui() override;

private:
    std::filesystem::path _curDirectory;
    std::filesystem::path _selectedFile;
    std::filesystem::path _assetsDirectory;

    struct CachedDir {
        std::vector<std::filesystem::directory_entry> directories;
        std::vector<std::filesystem::directory_entry> files;
        std::filesystem::file_time_type lastModified;
        bool valid = false;
    };

    std::unordered_map<std::filesystem::path, CachedDir, std::hash<std::filesystem::path>> _dirCache;

    void DrawDir(const std::filesystem::path& path, const std::filesystem::path& rootPath);
    bool CacheDirectory(const std::filesystem::path& path);
};

}
//*/