#include "SceneHierarchyPanel.h"
#include "OD/Editor/Editor.h"
#include "OD/Core/ImGui.h"
#include "OD/Scene/Scripts.h"
#include "OD/RenderPipeline/CameraComponent.h"
#include "OD/RenderPipeline/LightComponent.h"
#include "OD/RenderPipeline/MeshRendererComponent.h"
#include "OD/RenderPipeline/EnvironmentComponent.h"
#include "OD/Physics/PhysicsSystem.h"
#include "OD/Utils/PlatformUtils.h"
//#include "OD/AnimationSystem/Animator.h"
#include <glm/gtc/type_ptr.hpp>
#include <functional>
#include <string>
#include <imgui/imgui_internal.h>

namespace OD{

bool _toDestroy = false;
entt::entity _toDestroyEntity;

entt::entity _savePrefab = entt::null;

SceneHierarchyPanel::SceneHierarchyPanel(){
    name = "SceneHierarchyPanel";
    show = true;
}

void SceneHierarchyPanel::OnGui(){
    if(scene == nullptr) return;

    //if(ImGui::Begin("Scene Hierarchy")){
    ImGui::Begin("Scene Hierarchy");
        /*_scene->GetRegistry().sort<InfoComponent>([](const auto &lhs, const auto &rhs) {
            return lhs.id() < rhs.id();
        });*/

        scene->GetRegistry().sort<InfoComponent>([](const entt::entity lhs, const entt::entity rhs) {
            return lhs < rhs;
        });

        /*Vector3 camPos = scene->GetMainCamera2().GetComponent<TransformComponent>().Position();
        scene->GetRegistry().sort<TransformComponent>([&](const TransformComponent& lhs, const TransformComponent& rhs) {
            return math::distance(camPos, lhs.localPosition) < math::distance(camPos, rhs.localPosition);
        });*/

        auto view = scene->GetRegistry().view<TransformComponent, InfoComponent>();
        view.use<InfoComponent>();
        for(auto e: view){
            Entity _e(e, scene);
            DrawEntityNode(_e, true);
        }

        /*auto v = _scene->GetRegistry().view<entt::entity>();
        for(auto it = v.rbegin(), last = v.rend(); it != last; ++it){
            Entity _e(*it, _scene);
            DrawEntityNode(_e, true);
        }*/
        
        /*auto v = _scene->GetRegistry().view<entt::entity>();
        std::for_each(v.rbegin(), v.rend(), [&](entt::entity e){
            Entity _e(e, _scene);
            DrawEntityNode(_e, true);
        });*/

        /*_scene->GetRegistry().view<entt::entity>().each([&](auto e){
            Entity _e(e, _scene);
            DrawEntityNode(_e, true);
        });*/

        /*if(toDestroy.IsValid()){
            _scene->DestroyEntity(toDestroy.id());
            _editor->SetSelectionEntity(Entity());
            toDestroy = Entity();
            return;
        }*/
        
        if(_toDestroy){
            LogInfo("To Destroy Entity2: %d", _toDestroyEntity);
            editor->SetSelectionEntity(Entity());
            scene->DestroyEntity(_toDestroyEntity);
            _toDestroy = false;
            return;
        }

        if(ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()){
            //_editor->_selectionEntity = Entity();
            editor->SetSelectionEntity(Entity());
        }

        if(ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)){
            if(ImGui::MenuItem("Create Empty Entity")){
                scene->AddEntity("Empty Entity");
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
                        scene->CleanParent(targetEntity->Id());
                    }
                }
                ImGui::EndDragDropTarget();
            }

        }
        
        //ImGui::End();
        //ImGui::EndChild();
    //}
    ImGui::End();
}

void SceneHierarchyPanel::DrawEntityNode(Entity entity, bool root){
    Assert(entity.IsValid());

    TransformComponent& transform = entity.GetComponent<TransformComponent>();
    InfoComponent& info = entity.GetComponent<InfoComponent>();

    Entity children;

    if(root && transform.HasParent()) return;

    //ImGui::Text(info.name.c_str());

    ImGuiTreeNodeFlags flags = 
        ((entity == editor->selectionEntity) ? ImGuiTreeNodeFlags_Selected : 0) 
        | (transform.Children().empty() == false ? ImGuiTreeNodeFlags_OpenOnArrow : ImGuiTreeNodeFlags_Leaf);
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

    EntityType entityType = entity.GetComponent<InfoComponent>().Type();

    if(entityType == EntityType::PrefabRoot) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(55, 125, 205)));
    if(entityType == EntityType::PrefabChild) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(55, 155, 205)));

    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity.Id(), flags, info.name.c_str());
    
    if(entityType != EntityType::Stand) ImGui::PopStyleColor();
    
    if(entity.IsValid() && ImGui::BeginDragDropSource()){
        ImGui::SetDragDropPayload("EntityMoveDragDrop", &entity, sizeof(Entity), ImGuiCond_Once);
        ImGui::EndDragDropSource();
    }

    if(ImGui::IsItemClicked()){
        editor->SetSelectionEntity(entity);
    }

    if(ImGui::BeginDragDropTarget()){
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EntityMoveDragDrop");

        if(payload != nullptr){
            Entity* targetEntity = (Entity*)payload->Data;
            LogInfo("this: %s to: %s", entity.GetComponent<InfoComponent>().name.c_str(), targetEntity->GetComponent<InfoComponent>().name.c_str());
            if(entity.IsValid() && targetEntity->IsValid() && entity.Id() != targetEntity->Id()){
                children = *targetEntity;
            }
        }
        
        ImGui::EndDragDropTarget();
    }

    bool entityDeleted = false;
    if(ImGui::BeginPopupContextItem()){
        if(ImGui::MenuItem("Delete Entity")){
            entityDeleted = true;
            //_toDestroy = true;
            //_toDestroyEntity = entity.id();
        }
        if(entity.GetComponent<InfoComponent>().Type() == EntityType::Stand && ImGui::MenuItem("Save Prefab")){
            std::string path = FileDialogs::SaveFile("*.prefab");
            if(path.empty() == false){
                Scene* scene = SceneManager::Get().GetActiveScene();
                scene->Save(path.c_str(), entity.Id());
            } 
        }
        ImGui::EndPopup();
    }

    if(opened){
        for(auto i: transform.Children()){
            DrawEntityNode(Entity(i, entity.GetScene()), false);
        }

        ImGui::TreePop();
    }

    if(entityDeleted){
        LogInfo("To Destroy Entity: %d", entity.Id());

        scene->DestroyEntity(entity.Id());
        editor->SetSelectionEntity(Entity());

        //toDestroy = entity;
    } else if(children.IsValid()){
        scene->SetParent(entity.Id(), children.Id());
    }
}
}