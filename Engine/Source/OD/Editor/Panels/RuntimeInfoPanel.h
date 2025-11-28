#pragma once
#include "OD/Defines.h"
#include "OD/Editor/EditorPanel.h"

namespace OD{

class OD_API RuntimeInfoPanel: public EditorPanel{
public:
    RuntimeInfoPanel();
    void OnGui() override;
};

}