#include "SceneHierarchyPanel.h"
#include "OD/Core/ImGui.h"

namespace OD{

void SceneHierarchyPanel::OnGui(bool* showSceneHierarchy, bool* showInspector){
    if(_scene == nullptr) return;

    _selectionContext = Entity();

    if(ImGui::Begin("Scene Hierarchy", showSceneHierarchy)){
        auto view = _scene->GetRegistry().view<TransformComponent, InfoComponent>();
        for(auto e: view){
            Entity _e(e, _scene);
            DrawEntityNode(_e);

            _selectionContext = Entity(e, _scene);
        }
    }
    ImGui::End();

    ImGui::Begin("Properties", showInspector);
    if(_selectionContext.IsValid()){
        DrawComponents(_selectionContext);
    }
    ImGui::End();
}

void SceneHierarchyPanel::DrawEntityNode(Entity entity){
    TransformComponent& transform = entity.GetComponent<TransformComponent>();
    InfoComponent& info = entity.GetComponent<InfoComponent>();

    if(transform.hasParent()) return;

    ImGui::Text(info.name.c_str());
}

void SceneHierarchyPanel::DrawComponents(Entity entity){
    TransformComponent& transform = entity.GetComponent<TransformComponent>();
    InfoComponent& info = entity.GetComponent<InfoComponent>();

    ImGui::Text(info.name.c_str());
}

}