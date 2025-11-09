#pragma once
#include "OD/Defines.h"
#include "OD/Editor/EditorPanel.h"

namespace OD{

class OD_API GlobalSettingsPanel: public EditorPanel{
public:
    GlobalSettingsPanel();
    void OnGui() override;
};

}