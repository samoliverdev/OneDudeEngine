#pragma once
#include "OD/Defines.h"
#include "OD/Core/Module.h"
#include "OD/Core/Asset.h"
#include "OD/Scene/Scene.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Editor/Panels/SceneHierarchyPanel.h"
#include "OD/Editor/Panels/ContentBrowserPanel.h"
#include "OD/Editor/Panels/InspectorPanel.h"
#include "OD/Editor/Panels/ViewportPanel.h"
#include "OD/Editor/Panels/ProfilePanel.h"
#include "OD/Editor/Panels/RendererStatsPanel.h"
#include "OD/Graphics/Framebuffer.h"
#include "OD/Serialization/Serialization.h"
#include "EditorCamera.h"
#include "Workspace.h"

namespace OD{

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
        ar(
            CEREAL_NVP(sceneHierarchyPanel.show),
            CEREAL_NVP(contentBrowserPanel.show),
            CEREAL_NVP(inspectorPanel.show),
            CEREAL_NVP(viewportPanel.show),
            CEREAL_NVP(profilePanel.show),
            CEREAL_NVP(rendererStatsPanel.show)
        );
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