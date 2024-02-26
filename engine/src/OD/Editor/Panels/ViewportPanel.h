#pragma once
#include "OD/Defines.h"
#include "OD/Editor/EditorPanel.h"

namespace OD{

class OD_API ViewportPanel: public EditorPanel{
public:
    ViewportPanel();
    void OnGui() override;
};

}