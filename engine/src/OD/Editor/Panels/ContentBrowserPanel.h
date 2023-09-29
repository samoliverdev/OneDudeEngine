#pragma once

#include <filesystem>

namespace OD{

class Editor;

class ContentBrowserPanel{
public:
    ContentBrowserPanel();
    
    inline void SetEditor(Editor* editor){ _editor = editor; }
    void OnGui();

private:
    Editor* _editor;
    
    std::filesystem::path _curDirectory;
    std::filesystem::path _selectedFile;

    void DrawDir(std::filesystem::path path, std::filesystem::path rootPath);
};

}