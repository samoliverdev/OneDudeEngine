#pragma once

#include "OD/Core/Module.h"
#include "OD/Scene/Scene.h"
#include "OD/Editor/Panels/SceneHierarchyPanel.h"

namespace OD{

class Editor: public Module{
public:
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;

    enum class GizmosType{None, Translation, Rotation, Scale};

private:
    SceneHierarchyPanel _sceneHierarchyPanel;

    bool _showSceneHierarchy = true;
    bool _showInspector = true;
    bool _open = true;

    GizmosType _gizmoType;

    void DrawMainPanel();
    void DrawGizmos();
};

}