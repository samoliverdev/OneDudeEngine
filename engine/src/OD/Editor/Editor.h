#pragma once

#include "OD/Core/Module.h"
#include "OD/Scene/Scene.h"
#include "OD/Editor/Panels/SceneHierarchyPanel.h"
#include "OD/Renderer/Framebuffer.h"
#include "EditorCamera.h"

namespace OD{

class Editor: public Module{
public:
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;

    enum class GizmosType{None, Translation, Rotation, Scale};
    
    static inline Editor* Get(){ return instance; }

private:
    static Editor* instance;

    SceneHierarchyPanel _sceneHierarchyPanel;

    bool _showSceneHierarchy = true;
    bool _showInspector = true;
    bool _open = true;

    Vector2 _viewportSize;
    GizmosType _gizmoType;
    Framebuffer* _framebuffer;
    std::string _curScenePath;
    EditorCamera _cam;

    void HandleShotcuts();
    void PlayScene();
    void StopScene();
    void NewScene();
    void OpenScene();
    void SaveAsScene();

    void DrawMainMenuBar();

    void DrawMainPanel();
    void DrawMainWorkspace();
    
    void DrawGizmos();
};

}