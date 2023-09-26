#pragma once

#include <filesystem>

namespace OD{

class ContentBrowserPanel{
public:
    ContentBrowserPanel();

    void OnGui();

private:
    std::filesystem::path _curDirectory;
    std::filesystem::path _selectedFile;

    void DrawDir(std::filesystem::path path, std::filesystem::path rootPath);
};

}