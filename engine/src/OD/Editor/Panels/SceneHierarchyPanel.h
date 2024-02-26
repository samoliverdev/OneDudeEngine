#pragma once
#include "OD/Defines.h"
#include "OD/Scene/Scene.h"
#include "OD/Editor/EditorPanel.h"

namespace OD{

class OD_API SceneHierarchyPanel: public EditorPanel{
public:
    SceneHierarchyPanel();
    void OnGui() override;

private:
    Entity toDestroy;
    void DrawEntityNode(Entity entity, bool root);
};

}