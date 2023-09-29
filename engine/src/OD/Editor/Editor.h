#pragma once

#include "OD/Core/Module.h"
#include "OD/Core/Asset.h"
#include "OD/Scene/Scene.h"
#include "OD/Editor/Panels/SceneHierarchyPanel.h"
#include "OD/Editor/Panels/ContentBrowserPanel.h"
#include "OD/Editor/Panels/InspectorPanel.h"
#include "OD/Renderer/Framebuffer.h"
#include "EditorCamera.h"

namespace OD{

class Editor: public Module{
    friend class SceneHierarchyPanel;
    friend class ContentBrowserPanel;
    friend class InspectorPanel;

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
    ContentBrowserPanel _contentBrowserPanel;
    InspectorPanel _inspectorPanel;

    Entity _selectionEntity;
    Ref<Asset> _selectionAsset;
    bool _selectionOnAsset;

    bool _showSceneHierarchy = true;
    bool _showInspector = true;
    bool _open = true;

    Vector2 _viewportSize;
    GizmosType _gizmoType;
    Framebuffer* _framebuffer;
    std::string _curScenePath;
    EditorCamera _cam;

    ImVec2 m_ViewportBounds[2];

    inline void SetSelectionEntity(Entity entity){
        _selectionEntity = entity;
        _selectionOnAsset = false;
    }

    inline void SetSelectionAsset(Ref<Asset> asset){
        _selectionAsset = asset;
        _selectionOnAsset = true;
    }

    inline void UnselectAll(){
        _selectionEntity = Entity();
        _selectionAsset = nullptr;
        _selectionOnAsset = false;;
    }

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