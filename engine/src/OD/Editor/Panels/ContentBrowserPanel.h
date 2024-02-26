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