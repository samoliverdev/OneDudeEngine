#pragma once

#include <filesystem>
#include "OD/Editor/EditorPanel.h"

namespace OD{

class Editor;

class ContentBrowserPanel: public EditorPanel{
public:
    ContentBrowserPanel();
    void OnGui() override;

private:
    std::filesystem::path _curDirectory;
    std::filesystem::path _selectedFile;

    void DrawDir(std::filesystem::path path, std::filesystem::path rootPath);
};

}