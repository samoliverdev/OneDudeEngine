#pragma once

#include "OD/Scene/Scene.h"

namespace OD{

class SceneHierarchyPanel{
public:
    inline void SetScene(Scene* scene){ _scene = scene; }
    void OnGui(bool* showSceneHierarchy, bool* showInspector);
private:
    Scene* _scene;
    Entity _selectionContext;

    void DrawEntityNode(Entity entity);
    void DrawComponents(Entity entity);
};

}