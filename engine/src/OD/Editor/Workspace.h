#pragma once
#include "OD/Defines.h"
#include "OD/Scene/Scene.h"
#include "EditorPanel.h"

namespace OD{

class Editor;

class OD_API MainWorkspace: public EditorPanel {
    friend class Editor;
public:
    inline void AddPanel(EditorPanel* panel){ 
        panels.push_back(panel); 
    }

    inline void OnGui() override {
        for(auto i: panels){
            if(i->show == false) continue;

            i->SetEditor(editor);
            i->SetScene(scene);
            i->OnGui();
        }
    }

private:
    std::vector<EditorPanel*> panels;
};

}