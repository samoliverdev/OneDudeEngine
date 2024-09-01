#pragma once
#include "OD/Defines.h"
#include "OD/Core/Module.h"
#include "OD/Editor/Panels/SceneHierarchyPanel.h"
#include "OD/Editor/Panels/ContentBrowserPanel.h"
#include "OD/Editor/Panels/InspectorPanel.h"
#include "OD/Editor/Panels/ViewportPanel.h"
#include "OD/Editor/Panels/ProfilePanel.h"
#include "OD/Editor/Panels/RendererStatsPanel.h"
#include "OD/Serialization/Serialization.h"
#include "EditorCamera.h"
#include "Workspace.h"

namespace OD{

class Asset;
class Framebuffer;

class OD_API Editor: public Module{
    friend class SceneHierarchyPanel;
    friend class ContentBrowserPanel;
    friend class InspectorPanel;
    friend class ViewportPanel;

public:
    void OnInit() override;
    void OnExit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;

    enum class GizmosType{None, Translation, Rotation, Scale};
    
    static Editor* Get();

    inline void SetSelectionAsset(Ref<Asset> asset){
        selectionAsset = asset;
        selectionOnAsset = true;
    }

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDumpNVP(ar, sceneHierarchyPanel.show);
        ArchiveDumpNVP(ar, contentBrowserPanel.show);
        ArchiveDumpNVP(ar, inspectorPanel.show);
        ArchiveDumpNVP(ar, viewportPanel.show);
        ArchiveDumpNVP(ar, profilePanel.show);
        ArchiveDumpNVP(ar, rendererStatsPanel.show);
    }

private:
    //static Editor* instance;

    SceneHierarchyPanel sceneHierarchyPanel;
    ContentBrowserPanel contentBrowserPanel;
    InspectorPanel inspectorPanel;
    ViewportPanel viewportPanel;
    ProfilePanel profilePanel;
    RendererStatsPanel rendererStatsPanel;
    MainWorkspace mainWorkspace;

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