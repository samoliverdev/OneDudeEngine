#pragma once
#include "OD/Defines.h"
#include "OD/Editor/EditorPanel.h"

namespace OD{

class OD_API ProfilePanel: public EditorPanel{
public:
    enum class ViewMode{List, Tree};
    ViewMode viewMode;

    ProfilePanel();
    void OnGui() override;
};

}