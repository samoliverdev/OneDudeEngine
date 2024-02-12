#pragma once

#include "OD/Scene/Scene.h"
#include "EditorPanel.h"

namespace OD{

class Editor;

class MainWorkspace: public EditorPanel {
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