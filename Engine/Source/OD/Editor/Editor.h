#pragma once
#include "OD/Defines.h"
#include "OD/Core/Module.h"
#include "OD/Editor/Panels/SceneHierarchyPanel.h"
#include "OD/Editor/Panels/ContentBrowserPanel.h"
#include "OD/Editor/Panels/InspectorPanel.h"
#include "OD/Editor/Panels/ViewportPanel.h"
#include "OD/Editor/Panels/ProfilePanel.h"
#include "OD/Editor/Panels/RendererStatsPanel.h"
#include "OD/Editor/Panels/GlobalSettingsPanel.h"
#include "OD/Editor/Panels/RuntimeInfoPanel.h"
#include "OD/Serialization/Serialization.h"
#include "EditorCamera.h"
#include "Workspace.h"
#include <functional>

namespace OD{

class Asset;
class Framebuffer;

class OD_API Editor: public Module{
    friend class SceneHierarchyPanel;
    friend class ContentBrowserPanel;
    friend class InspectorPanel;
    friend class ViewportPanel;

public:
    Editor(bool indrawSceneToCustomFramebuffer = true):drawSceneToCustomFramebuffer(indrawSceneToCustomFramebuffer){ name = "Editor"; }

    void OnInit() override;
    void OnExit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;

    enum class GizmosType{None, Translation, Rotation, Scale};
    
    static Editor* Get();

    inline EditorCamera& EditorCam(){ return editorCam; }

    void AddMenuCommand(const std::string path, std::function<void()> command);

    inline Entity GetSelectionEntity(){ return selectionEntity; }
    inline const std::unordered_set<Entity>& GetSelectedEntities() const { return _selectedEntities; }

    inline void SetSelectionAsset(Ref<Asset> asset){
        selectionAsset = asset;
        selectionOnAsset = true;
    }

    inline void UnselectAll(){
        selectionEntity = EntityNull;
        selectedEntities.clear();
        _selectedEntities.clear();

        selectionAsset = nullptr;
        selectionOnAsset = false;
    }

    inline void AddCustomPanel(EditorPanel* panel){
        mainWorkspace.AddPanel(panel);
    }

    inline Framebuffer* AssetPreviewFramebuffer(){ return assetPreviewFramebuffer; }

    void SetModelAssetPreview(Ref<Model> model);
    void SetModelAssetPreview(const std::string& path);
    void SetPrefabAssetPreview(Ref<Prefab> prefab);
    void SetPrefabAssetPreview(const std::string& path);

    inline bool DrawSceneToCustomFramebuffer(){ return drawSceneToCustomFramebuffer; }

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDumpNVP(ar, sceneHierarchyPanel.show);
        ArchiveDumpNVP(ar, contentBrowserPanel.show);
        ArchiveDumpNVP(ar, inspectorPanel.show);
        ArchiveDumpNVP(ar, viewportPanel.show);
        ArchiveDumpNVP(ar, profilePanel.show);
        ArchiveDumpNVP(ar, rendererStatsPanel.show);
        ArchiveDumpNVP(ar, globalSettingsPanel.show);
    }

    int ExecutionSortPriority() override;

private:
    //static Editor* instance;

    SceneHierarchyPanel sceneHierarchyPanel;
    ContentBrowserPanel contentBrowserPanel;
    InspectorPanel inspectorPanel;
    ViewportPanel viewportPanel;
    ProfilePanel profilePanel;
    RendererStatsPanel rendererStatsPanel;
    GlobalSettingsPanel globalSettingsPanel; 
    RuntimeInfoPanel runtimeInfoPanel;
    MainWorkspace mainWorkspace;

    Entity selectionEntity = EntityNull;
    std::vector<Entity> selectedEntities;
    std::unordered_set<Entity> _selectedEntities;

    Ref<Asset> selectionAsset;
    bool selectionOnAsset;

    //bool showSceneHierarchy = true;
    //bool showInspector = true;
    bool open = true;
    bool drawSceneToCustomFramebuffer = true;

    struct GizmoInteractionState{
        bool active = false;
        bool isUsing;
        bool isOver;
    };
    GizmoInteractionState gizmoInteractionState;

    struct TransformChangeData{
        TransformComponent oldTrans;
        TransformComponent newTrans;
    };
    std::unordered_map<Entity, TransformChangeData> transformChangesData;

    //bool isOnManipulationGizmos = false;

    Vector2 viewportSize;
    GizmosType gizmoType;
    Framebuffer* framebuffer;
    std::string curScenePath;
    EditorCamera editorCam;

    ImVec2 viewportBounds[2];

    Scene* assetPreviewScene;
    AssetPreviewCamera assetPrevieweCam;
    Framebuffer* assetPreviewFramebuffer;
    Entity assetPreviewEntity = EntityNull;
    Ref<Model> lastModelAssetPreview;
    Ref<Prefab> lastPrefabAssetPreview;

    struct SnapSettings{
        bool enable = false;
        float posGridSize = 0.25f;
        float rotSnapAngle = 15;
    };
    SnapSettings snapSettings;

    enum class GizmoPivotMode { Pivot, Center };
    GizmoPivotMode pivotMode = GizmoPivotMode::Center;

    enum class GizmoSpace { Local, Global };
    GizmoSpace gizmoSpace = GizmoSpace::Local;

    inline void SetSelectionEntity(Entity entity){
        entity = GetTargetSelected(entity);

        selectionEntity = entity;
        selectedEntities.clear();
        _selectedEntities.clear();
        selectedEntities.push_back(entity);
        _selectedEntities.insert(entity);

        selectionOnAsset = false;
    }

    inline void AddSelectionEntity(Entity entity){
        entity = GetTargetSelected(entity);
        
        selectionEntity = entity;
        selectedEntities.push_back(entity);
        _selectedEntities.insert(entity);

        selectionOnAsset = false;
    }

    inline int SelectedEntitiesCount(){ return selectedEntities.size(); }

    Entity GetTargetSelected(Entity input);
    
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