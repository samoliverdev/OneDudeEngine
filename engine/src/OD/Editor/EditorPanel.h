#pragma once

namespace OD{

class Scene;
class Editor;

class OD_API EditorPanel{
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