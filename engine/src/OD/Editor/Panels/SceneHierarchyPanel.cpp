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
#include <imgui/imgui_internal.h>

namespace OD{

void SceneHierarchyPanel::OnGui(){
    if(_scene == nullptr) return;

    if(ImGui::Begin("Scene Hierarchy")){
        /*for(auto e: _scene->GetRegistry().view<TransformComponent, InfoComponent>()){
            Entity _e(e, _scene);
            DrawEntityNode(_e, true);
        }*/

        /*auto v = _scene->GetRegistry().view<entt::entity>();
        for(auto it = v.rbegin(), last = v.rend(); it != last; ++it){
            Entity _e(*it, _scene);
            DrawEntityNode(_e, true);
        }*/
        
        /*auto v = _scene->GetRegistry().view<entt::entity>();
        std::for_each(v.rbegin(), v.rend(), [&](auto e){
            Entity _e(e, _scene);
            DrawEntityNode(_e, true);
        });*/

        _scene->GetRegistry().view<entt::entity>().each([&](auto e){
            Entity _e(e, _scene);
            DrawEntityNode(_e, true);
        });

        /*if(toDestroy.IsValid()){
            _scene->DestroyEntity(toDestroy.id());
            _editor->SetSelectionEntity(Entity());
            toDestroy = Entity();
            return;
        }*/

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

        /*ImGui::Button("test", ImGui::GetContentRegionAvail());
        if(ImGui::BeginDragDropTarget()){
            const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EntityMoveDragDrop");
            if(payload != nullptr){
                Entity* path = (Entity*)payload->Data;
                LogInfo("this: %s", path->GetComponent<InfoComponent>().name.c_str());
                if(path->IsValid()){}
            }
            ImGui::EndDragDropTarget();
        }*/

        if(ImGui::IsDragDropActive()){

        ImGui::BeginChild("BottomBar", ImVec2(0,0), false, 0); // Use avail width/height
            //ImGui::Text("Footer");
            ImGui::EndChild();
            if(ImGui::BeginDragDropTarget()){
                const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EntityMoveDragDrop");
                if(payload != nullptr){
                    Entity* targetEntity = (Entity*)payload->Data;
                    LogInfo("this: %s", targetEntity->GetComponent<InfoComponent>().name.c_str());
                    if(targetEntity->IsValid()){
                        _scene->CleanParent(targetEntity->id());
                    }
                }
                ImGui::EndDragDropTarget();
            }

        }
        
        ImGui::End();
    }
}

void SceneHierarchyPanel::DrawEntityNode(Entity entity, bool root){
    Assert(entity.IsValid());

    TransformComponent& transform = entity.GetComponent<TransformComponent>();
    InfoComponent& info = entity.GetComponent<InfoComponent>();

    Entity children;

    if(root && transform.hasParent()) return;

    //ImGui::Text(info.name.c_str());

    ImGuiTreeNodeFlags flags = 
        ((entity == _editor->_selectionEntity) ? ImGuiTreeNodeFlags_Selected : 0) 
        | (transform.children().empty() == false ? ImGuiTreeNodeFlags_OpenOnArrow : ImGuiTreeNodeFlags_Leaf);
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity.id(), flags, info.name.c_str());
    
    if(entity.IsValid() && ImGui::BeginDragDropSource()){
        ImGui::SetDragDropPayload("EntityMoveDragDrop", &entity, sizeof(Entity), ImGuiCond_Once);
        ImGui::EndDragDropSource();
    }

    if(ImGui::IsItemClicked()){
        _editor->SetSelectionEntity(entity);
    }

    if(ImGui::BeginDragDropTarget()){
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EntityMoveDragDrop");

        if(payload != nullptr){
            Entity* targetEntity = (Entity*)payload->Data;
            LogInfo("this: %s to: %s", entity.GetComponent<InfoComponent>().name.c_str(), targetEntity->GetComponent<InfoComponent>().name.c_str());
            if(entity.IsValid() && targetEntity->IsValid() && entity.id() != targetEntity->id()){
                children = *targetEntity;
            }
        }
        
        ImGui::EndDragDropTarget();
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
        LogInfo("To Destroy Entity: %d", entity.id());

        _scene->DestroyEntity(entity.id());
        _editor->SetSelectionEntity(Entity());

        //toDestroy = entity;
    } else if(children.IsValid()){
        _scene->SetParent(entity.id(), children.id());
    }
}
}