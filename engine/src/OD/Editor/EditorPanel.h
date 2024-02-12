#pragma once

#include "OD/Scene/Scene.h"

namespace OD{

class Editor;

class EditorPanel{
public:
    inline void SetEditor(Editor* inEditor){ editor = inEditor; }
    inline void SetScene(Scene* inScene){ scene = inScene; }
    virtual void OnGui(){}

    bool show = true;
    std::string name;

protected:
    Scene* scene;
    Editor* editor;
};

}