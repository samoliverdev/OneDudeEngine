#pragma once

#include "OD/Scene/Scene.h"

namespace OD{

class SceneHierarchyPanel{
public:
    inline void SetEditor(Editor* editor){ _editor = editor; }
    inline void SetScene(Scene* scene){ _scene = scene; }

    void OnGui();

private:
    Scene* _scene;
    Editor* _editor;

    Entity toDestroy;

    void DrawEntityNode(Entity entity, bool root);
};

}