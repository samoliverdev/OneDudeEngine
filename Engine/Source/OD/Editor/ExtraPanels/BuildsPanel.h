#pragma once
#include "OD/Editor/Editor.h"

namespace OD{

class OD_API BuildsPanel: public EditorPanel{
public:
    std::string buildPath;
    std::string packZipName = "Game.pak";
    std::vector<std::string> dontBuildAssetFolders = {"Engine/", "Standard/"};
    std::vector<std::string> dontZipAssetFolders = {"Engine/", "Standard/"};
    
    BuildsPanel();
    void OnGui() override;
private:
    void Build();
};

}