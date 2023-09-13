#include "SceneHierarchyPanel.h"
#include "OD/Core/ImGui.h"
#include <glm/gtc/type_ptr.hpp>

namespace OD{

void SceneHierarchyPanel::OnGui(bool* showSceneHierarchy, bool* showInspector){
    if(_scene == nullptr) return;

    //_selectionContext = Entity();

    if(ImGui::Begin("Scene Hierarchy", showSceneHierarchy)){
        auto view = _scene->GetRegistry().view<TransformComponent, InfoComponent>();
        for(auto e: view){
            Entity _e(e, _scene);
            DrawEntityNode(_e, true);
        }
    }

    if(ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()){
        _selectionContext = Entity();
    }

    ImGui::End();

    ImGui::Begin("Properties", showInspector);
    if(_selectionContext.IsValid()){
        DrawComponents(_selectionContext);
    }
    ImGui::End();
}

void SceneHierarchyPanel::DrawEntityNode(Entity entity, bool root){
    TransformComponent& transform = entity.GetComponent<TransformComponent>();
    InfoComponent& info = entity.GetComponent<InfoComponent>();

    if(root && transform.hasParent()) return;

    //ImGui::Text(info.name.c_str());

    ImGuiTreeNodeFlags flags = 
        ((entity == _selectionContext) ? ImGuiTreeNodeFlags_Selected : 0) 
        | (transform.children().empty() == false ? ImGuiTreeNodeFlags_OpenOnArrow : ImGuiTreeNodeFlags_Leaf);

    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity.id(), flags, info.name.c_str());
    
    if(ImGui::IsItemClicked()){
        _selectionContext = entity;
    }

    if(opened){
        for(auto i: transform.children()){
            DrawEntityNode(Entity(i, entity.scene()), false);
        }

        ImGui::TreePop();
    }
}

void SceneHierarchyPanel::DrawComponents(Entity entity){
    TransformComponent& transform = entity.GetComponent<TransformComponent>();
    InfoComponent& info = entity.GetComponent<InfoComponent>();

    if(ImGui::TreeNodeEx((void*)typeid(InfoComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Info")){
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strcpy_s(buffer, sizeof(buffer), info.name.c_str());
        if(ImGui::InputText("Name", buffer, sizeof(buffer))){
            info.name = std::string(buffer);
        }

        ImGui::TreePop();
    }

    if(ImGui::TreeNodeEx((void*)typeid(TransformComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Transform")){
        float p[] = {transform.localPosition().x, transform.localPosition().y, transform.localPosition().z};
        if(ImGui::DragFloat3("Position", p, 0.5f)){
            transform.localPosition(Vector3(p[0], p[1], p[2]));
        }  

        ImGui::TreePop();
    }
}

}