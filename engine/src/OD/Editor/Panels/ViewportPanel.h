#pragma once

#include "OD/Editor/EditorPanel.h"

namespace OD{

class ViewportPanel: public EditorPanel{
public:
    ViewportPanel();
    void OnGui() override;
};

}