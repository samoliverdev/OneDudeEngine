#pragma once
#include "OD/Defines.h"
#include "OD/Editor/EditorPanel.h"

namespace OD{

class OD_API RendererStatsPanel: public EditorPanel{
public: 
    RendererStatsPanel();
    void OnGui() override;
};

}