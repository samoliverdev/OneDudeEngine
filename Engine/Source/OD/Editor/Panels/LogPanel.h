#pragma once
#include "OD/Defines.h"
#include "OD/Editor/EditorPanel.h"

namespace OD{

class OD_API LogPanel: public EditorPanel{
public:
    LogPanel();
    void OnGui() override;
private:
    bool showInfo    = true;
    bool showWarning = true;
    bool showError   = true;
    bool showFatal   = true;
    bool autoScroll  = true;
};

}