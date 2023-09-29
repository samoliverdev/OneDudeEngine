#include "SceneHierarchyPanel.h"
#include "OD/Editor/Editor.h"
#include "OD/Core/ImGui.h"
#include "OD/Scene/Scripts.h"
#include "OD/RendererSystem/CameraComponent.h"
#include "OD/RendererSystem/LightComponent.h"
#include "OD/RendererSystem/MeshRendererComponent.h"
#include "OD/RendererSystem/EnvironmentComponent.h"
#include "OD/PhysicsSystem/PhysicsSystem.h"
#include "OD/AnimationSystem/Animator.h"
#include <glm/gtc/type_ptr.hpp>
#include <functional>
#include <string>

namespace OD{

void SceneHierarchyPanel::OnGui(){
    if(_scene == nullptr) return;

    if(ImGui::Begin("Scene Hierarchy")){
        auto view = _scene->GetRegistry().view<TransformComponent, InfoComponent>();
        for(auto e: view){
            Entity _e(e, _scene);
            DrawEntityNode(_e, true);
        }
    
        if(ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()){
            //_editor->_selectionEntity = Entity();
            _editor->SetSelectionEntity(Entity());
        }

        if(ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)){
            if(ImGui::MenuItem("Create Empty Entity")){
                _scene->AddEntity("Empty Entity");
            }
            ImGui::EndPopup();
        }

        ImGui::End();
    }
}

void SceneHierarchyPanel::DrawEntityNode(Entity entity, bool root){
    TransformComponent& transform = entity.GetComponent<TransformComponent>();
    InfoComponent& info = entity.GetComponent<InfoComponent>();

    if(root && transform.hasParent()) return;

    //ImGui::Text(info.name.c_str());

    ImGuiTreeNodeFlags flags = 
        ((entity == _editor->_selectionEntity) ? ImGuiTreeNodeFlags_Selected : 0) 
        | (transform.children().empty() == false ? ImGuiTreeNodeFlags_OpenOnArrow : ImGuiTreeNodeFlags_Leaf);
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity.id(), flags, info.name.c_str());
    
    if(ImGui::IsItemClicked()){
        //_editor->_selectionEntity = entity;
        _editor->SetSelectionEntity(entity);
    }

    bool entityDeleted = false;
    if(ImGui::BeginPopupContextItem()){
        if(ImGui::MenuItem("Delete Entity")){
            entityDeleted = true;
        }
        ImGui::EndPopup();
    }

    if(opened){
        for(auto i: transform.children()){
            DrawEntityNode(Entity(i, entity.scene()), false);
        }

        ImGui::TreePop();
    }

    if(entityDeleted){
        _scene->DestroyEntity(entity.id());
        //_editor->_selectionEntity = Entity();
        _editor->SetSelectionEntity(Entity());
        //if(_selectionContext == entity) _selectionContext = Entity();
    }
}
}