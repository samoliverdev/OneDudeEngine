#pragma once
#include "OD/Editor/Editor.h"

namespace OD{

class OD_API BuildsPanel: public EditorPanel{
public:
    std::string buildPath;
    
    BuildsPanel();
    void OnGui() override;
private:
    void Build();
};

}