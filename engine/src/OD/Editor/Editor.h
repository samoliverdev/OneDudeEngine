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

    inline void SetSelectionAsset(Ref<Asset> asset){
        selectionAsset = asset;
        selectionOnAsset = true;
    }

private:
    static Editor* instance;

    SceneHierarchyPanel sceneHierarchyPanel;
    ContentBrowserPanel contentBrowserPanel;
    InspectorPanel inspectorPanel;

    Entity selectionEntity;
    Ref<Asset> selectionAsset;
    bool selectionOnAsset;

    bool showSceneHierarchy = true;
    bool showInspector = true;
    bool open = true;

    Vector2 viewportSize;
    GizmosType gizmoType;
    Framebuffer* framebuffer;
    std::string curScenePath;
    EditorCamera editorCam;

    ImVec2 viewportBounds[2];

    inline void SetSelectionEntity(Entity entity){
        selectionEntity = entity;
        selectionOnAsset = false;
    }

    inline void UnselectAll(){
        selectionEntity = Entity();
        selectionAsset = nullptr;
        selectionOnAsset = false;;
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