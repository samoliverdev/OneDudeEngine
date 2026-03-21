#pragma once
#include "OD/Defines.h"
#include "OD/Scene/ECS.h"
#include "OD/Editor/EditorPanel.h"

namespace OD{

class OD_API SceneHierarchyPanel: public EditorPanel{
public:
    SceneHierarchyPanel();
    void OnGui() override;

private:
    bool showHide = false;

    Entity toDestroy;
    void DrawEntityNode(Entity entity, bool root);
};

}