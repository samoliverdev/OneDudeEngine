#pragma once
#include "OD/Defines.h"
#include "OD/Editor/EditorPanel.h"
#include "OD/Core/Instrumentor.h"

namespace OD{

class OD_API ProfilePanel: public EditorPanel{
public:
    ProfilePanel();
    void OnGui() override;
};

}