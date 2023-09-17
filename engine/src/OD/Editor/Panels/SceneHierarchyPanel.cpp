#include "SceneHierarchyPanel.h"
#include "OD/Core/ImGui.h"
#include "OD/Scene/Scripts.h"
#include "OD/RendererSystem/CameraComponent.h"
#include "OD/RendererSystem/LightComponent.h"
#include "OD/RendererSystem/MeshRendererComponent.h"
#include "OD/PhysicsSystem/PhysicsSystem.h"
#include "OD/AnimationSystem/Animator.h"
#include <glm/gtc/type_ptr.hpp>
#include <functional>
#include <string>

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

    if(ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)){
        if(ImGui::MenuItem("Create Empty Entity")){
            _scene->AddEntity("Empty Entity");
        }
        ImGui::EndPopup();
    }

    ImGui::End();

    ImGui::Begin("Properties", showInspector);
    if(_selectionContext.IsValid()){
        DrawComponents(_selectionContext);
        ImGui::Separator();
        ImGui::Spacing();
        ShowAddComponent(_selectionContext);
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
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity.id(), flags, info.name.c_str());
    
    if(ImGui::IsItemClicked()){
        _selectionContext = entity;
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
        if(_selectionContext == entity){
            _selectionContext = Entity();
        }
    }
}

template<typename T>
void DrawComponent(Entity e, const char* name){
    const ImGuiTreeNodeFlags treeNodeFlags = 
        ImGuiTreeNodeFlags_DefaultOpen 
        | ImGuiTreeNodeFlags_Framed 
        | ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding;

    if(e.HasComponent<T>()){
        bool removeComponent = false;

        //ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name);
        //ImGui::SameLine(ImGui::GetWindowWidth() - 25.0f);
        //if(ImGui::Button("+", ImVec2(20, 20))){
        //    ImGui::OpenPopup("ComponentSettings");
        //}
        //ImGui::PopStyleVar();
        //if(ImGui::BeginPopup("ComponentSettings")){
        //    if(ImGui::MenuItem("Remove Component")){
        //        removeComponent = true;
        //    }
        //    ImGui::EndPopup();
        //}

        if(ImGui::BeginPopupContextItem()){
            if(ImGui::MenuItem("Remove Component")){
                removeComponent = true;
            }
            ImGui::EndPopup();
        }

        if(open){
            T::OnGui(e);
            ImGui::TreePop();
        }

        if(removeComponent){
            e.RemoveComponent<T>();
        }
    }
}

template<typename T, typename UIFunction>
void DrawComponent(Entity e, const char* name, UIFunction function){
    const ImGuiTreeNodeFlags treeNodeFlags = 
        ImGuiTreeNodeFlags_DefaultOpen 
        | ImGuiTreeNodeFlags_Framed 
        | ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding;

    if(e.HasComponent<T>()){
        bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name);

        if(open){
            function(e);
            ImGui::TreePop();
        }
    }
}

void SceneHierarchyPanel::DrawArchive(Archive& ar){
    const ImGuiTreeNodeFlags treeNodeFlags = 
        //ImGuiTreeNodeFlags_DefaultOpen 
        //| ImGuiTreeNodeFlags_Framed 
            ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding;

    
    std::hash<std::string> hasher;

    for(auto i: ar.values()){
        if(i.type == ArchiveValue::Type::Float){
            ImGui::DragFloat(i.name.c_str(), i.floatValue);
        }
        if(i.type == ArchiveValue::Type::String){
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy_s(buffer, sizeof(buffer), i.stringValue->c_str());
            if(ImGui::InputText(i.name.c_str(), buffer, sizeof(buffer))){
                *i.stringValue = std::string(buffer);
            }
        }

        if(i.type == ArchiveValue::Type::T){
            if(ImGui::TreeNodeEx((void*)hasher(i.name), treeNodeFlags, i.name.c_str())){
                DrawArchive(i.children[0]);
                ImGui::TreePop();
            }
        }

        if(i.type == ArchiveValue::Type::TList){
            if(ImGui::TreeNodeEx((void*)hasher(i.name), treeNodeFlags, i.name.c_str())){
                int index = 0;
                for(auto j: i.children){
                    if(ImGui::TreeNodeEx((void*)(hasher(i.name)+index), treeNodeFlags, std::to_string(index).c_str())){
                        DrawArchive(j);
                        ImGui::TreePop();
                    }
                    index += 1;
                }
                ImGui::TreePop();
            }
        }
    }
}

void SceneHierarchyPanel::DrawComponentFromSerializeFuncs(Entity e, std::string name, SceneManager::SerializeFuncs &sf){
    const ImGuiTreeNodeFlags treeNodeFlags = 
        ImGuiTreeNodeFlags_DefaultOpen 
        | ImGuiTreeNodeFlags_Framed 
        | ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding;

    if(sf.hasComponent(e)){
        std::hash<std::string> hasher;
        bool open = ImGui::TreeNodeEx((void*)hasher(name), treeNodeFlags, name.c_str());
        if(open){
            Archive ar;
            sf.serialize(e, ar);
            DrawArchive(ar);

            ImGui::TreePop();
        }
    }
}

void SceneHierarchyPanel::DrawComponents(Entity entity){
    TransformComponent& transform = entity.GetComponent<TransformComponent>();
    InfoComponent& info = entity.GetComponent<InfoComponent>();

    DrawComponent<InfoComponent>(entity, "Info", [&](Entity e){
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strcpy_s(buffer, sizeof(buffer), info.name.c_str());
        if(ImGui::InputText("Name", buffer, sizeof(buffer))){
            info.name = std::string(buffer);
        }
    });

    DrawComponent<TransformComponent>(entity, "Transform", [&](Entity e){
        //if(e.HasComponent<RigidbodyComponent>()){
        //    RigidbodyComponent& rb = e.GetComponent<RigidbodyComponent>();
        //    float p[] = {rb.position().x, rb.position().y, rb.position().z};
        //    if(ImGui::DragFloat3("Position", p, 0.5f)){
        //        rb.position(Vector3(p[0], p[1], p[2]));
        //    }
        //} else {
            float p[] = {transform.localPosition().x, transform.localPosition().y, transform.localPosition().z};
            if(ImGui::DragFloat3("Position", p, 0.5f)){
                transform.localPosition(Vector3(p[0], p[1], p[2]));
            }
        //}  

        float r[] = {transform.localEulerAngles().x, transform.localEulerAngles().y, transform.localEulerAngles().z};
        if(ImGui::DragFloat3("Rotation", r, 0.5f)){
            transform.localEulerAngles(Vector3(r[0], r[1], r[2]));
        }  

        float s[] = {transform.localScale().x, transform.localScale().y, transform.localScale().z};
        if(ImGui::DragFloat3("Scale", s, 0.5f)){
            transform.localScale(Vector3(s[0], s[1], s[2]));
        } 
    });

    ImGui::Spacing(); ImGui::Spacing();

    DrawComponent<CameraComponent>(entity, "Camera");
    DrawComponent<LightComponent>(entity, "Light");
    DrawComponent<AnimatorComponent>(entity, "Animator");
    DrawComponent<MeshRendererComponent>(entity, "MeshRenderer");
    DrawComponent<RigidbodyComponent>(entity, "Rigidbody");
    DrawComponent<ScriptComponent>(entity, "Script");

    ImGui::Spacing(); ImGui::Spacing();

    for(auto& i: SceneManager::Get()._serializeFuncs){
        DrawComponentFromSerializeFuncs(entity, i.first, i.second);
    }
}

void SceneHierarchyPanel::ShowAddComponent(Entity entity){
    if(ImGui::Button("Add Component"))
        ImGui::OpenPopup("AddComponent");

    if(ImGui::BeginPopup("AddComponent")){
        if(ImGui::MenuItem("LightComponent")){
            _selectionContext.AddOrGetComponent<LightComponent>();
            ImGui::CloseCurrentPopup();
        }
        if(ImGui::MenuItem("CameraComponent")){
            _selectionContext.AddOrGetComponent<CameraComponent>();
            ImGui::CloseCurrentPopup();
        }
        if(ImGui::MenuItem("RigidbodyComponent")){
            _selectionContext.AddOrGetComponent<RigidbodyComponent>();
            ImGui::CloseCurrentPopup();
        }
        if(ImGui::MenuItem("MeshRendererComponent")){
            _selectionContext.AddOrGetComponent<MeshRendererComponent>();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

}