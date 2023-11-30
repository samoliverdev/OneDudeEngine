#include "InspectorPanel.h"
#include "OD/Editor/Editor.h"
#include "OD/Core/ImGui.h"
#include "OD/Scene/Scripts.h"
#include "OD/RendererSystem/CameraComponent.h"
#include "OD/RendererSystem/LightComponent.h"
#include "OD/RendererSystem/MeshRendererComponent.h"
#include "OD/RendererSystem/EnvironmentComponent.h"
#include "OD/PhysicsSystem/PhysicsSystem.h"
//#include "OD/AnimationSystem/Animator.h"
#include <glm/gtc/type_ptr.hpp>
#include <functional>
#include <string>

namespace OD{

void InspectorPanel::OnGui(){
    if(_scene == nullptr) return;

    //if(ImGui::Begin("Inspector")){
    ImGui::Begin("Inspector");
    if(_editor->_selectionEntity.IsValid() && _editor->_selectionOnAsset == false){
        DrawComponents(_editor->_selectionEntity);
        ImGui::Separator();
        ImGui::Spacing();
        ShowAddComponent(_editor->_selectionEntity);
    } else if(_editor->_selectionOnAsset == true && _editor->_selectionAsset != nullptr){
        _editor->_selectionAsset->OnGui();
    }
    ImGui::End();
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

void InspectorPanel::DrawArchive(ArchiveNode& ar){
    ArchiveNode::DrawArchive(ar);

    /*const ImGuiTreeNodeFlags treeNodeFlags = 
        //ImGuiTreeNodeFlags_DefaultOpen 
        //| ImGuiTreeNodeFlags_Framed 
            ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding;

    
    std::hash<std::string> hasher;

    for(auto i: ar.values){
        if(i.second.type == ArchiveNode::Type::Float){
            ImGui::DragFloat(i.first.c_str(), static_cast<float*>(i.second.value));
        }
        if(i.second.type == ArchiveNode::Type::String){
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy_s(buffer, sizeof(buffer), i.second.stringValue.c_str());
            if(ImGui::InputText(i.first.c_str(), buffer, sizeof(buffer))){
                i.second.stringValue = std::string(buffer);
            }
        }

        if(i.second.type == ArchiveNode::Type::Object){
            if(ImGui::TreeNodeEx((void*)hasher(i.first), treeNodeFlags, i.first.c_str())){
                DrawArchive(i.second);
                ImGui::TreePop();
            }
        }

        if(i.second.type == ArchiveNode::Type::List){
            if(ImGui::TreeNodeEx((void*)hasher(i.first), treeNodeFlags, i.first.c_str())){
                int index = 0;
                for(auto j: i.second.values){
                    if(ImGui::TreeNodeEx((void*)(hasher(i.first)+index), treeNodeFlags, std::to_string(index).c_str())){
                        DrawArchive(j.second);
                        ImGui::TreePop();
                    }
                    index += 1;
                }
                ImGui::TreePop();
            }
        }
    }*/
}

void InspectorPanel::DrawComponentFromCoreComponents(Entity e, std::string name, SceneManager::CoreComponent &f){
    std::hash<std::string> hasher;

    const ImGuiTreeNodeFlags treeNodeFlags = 
        ImGuiTreeNodeFlags_DefaultOpen 
        | ImGuiTreeNodeFlags_Framed 
        | ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding;

    if(f.hasComponent(e)){
        bool removeComponent = false;

        bool open = ImGui::TreeNodeEx((void*)hasher(name), treeNodeFlags, name.c_str());

        if(ImGui::BeginPopupContextItem()){
            if(ImGui::MenuItem("Remove Component")){
                removeComponent = true;
            }
            ImGui::EndPopup();
        }

        if(open){
            f.onGui(e);
            ImGui::TreePop();
        }

        if(removeComponent){
            //e.RemoveComponent<T>();
            f.removeComponent(e);
        }
    }
}

void InspectorPanel::DrawComponentFromSerializeFuncs(Entity e, std::string name, SceneManager::SerializeFuncs &sf){
    const ImGuiTreeNodeFlags treeNodeFlags = 
        ImGuiTreeNodeFlags_DefaultOpen 
        | ImGuiTreeNodeFlags_Framed 
        | ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding;

    if(sf.hasComponent(e)){
        bool removeComponent = false;
        std::hash<std::string> hasher;

        bool open = ImGui::TreeNodeEx((void*)hasher(name), treeNodeFlags, name.c_str());

        if(ImGui::BeginPopupContextItem()){
            if(ImGui::MenuItem("Remove Component")){
                removeComponent = true;
            }
            ImGui::EndPopup();
        }

        if(open){
            ArchiveNode ar(ArchiveNode::Type::Object, "", nullptr);
            sf.serialize(e, ar);
            ArchiveNode::DrawArchive(ar);

            ImGui::TreePop();
        }

        if(removeComponent){
            //e.RemoveComponent<T>();
            sf.removeComponent(e);
        }
    }
}

void InspectorPanel::DrawComponents(Entity entity){
    TransformComponent& transform = entity.GetComponent<TransformComponent>();
    InfoComponent& info = entity.GetComponent<InfoComponent>();

    DrawComponent<InfoComponent>(entity, "Info", [&](Entity e){
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        
        strcpy_s(buffer, sizeof(buffer), info.name.c_str());
        if(ImGui::InputText("Name", buffer, sizeof(buffer))){
            info.name = std::string(buffer);
        }

        strcpy_s(buffer, sizeof(buffer), info.tag.c_str());
        if(ImGui::InputText("Tag", buffer, sizeof(buffer))){
            info.tag = std::string(buffer);
        }

        ImGui::Text("Id: %zd", (size_t)e.id());
    });

    DrawComponent<TransformComponent>(entity, "Transform", [&](Entity e){
        /*if(e.HasComponent<RigidbodyComponent>()){
            RigidbodyComponent& rb = e.GetComponent<RigidbodyComponent>();
            float p[] = {rb.position().x, rb.position().y, rb.position().z};
            if(ImGui::DragFloat3("Position", p, 0.5f)){
                rb.position(Vector3(p[0], p[1], p[2]));
            }
        } else {*/
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

    ImGui::Spacing();
    //ImGui::Separator();
    ImGui::Spacing();
    
    for(auto& i: SceneManager::Get()._coreComponentsSerializer){
        //LogInfo("%s", i.first.c_str());
        DrawComponentFromCoreComponents(entity, i.first, i.second);
    }

    ImGui::Spacing();
    //ImGui::Separator();
    ImGui::Spacing(); 

    for(auto& i: SceneManager::Get()._componentsSerializer){
        DrawComponentFromSerializeFuncs(entity, i.first, i.second);
    }
}

void InspectorPanel::ShowAddComponent(Entity entity){
    if(ImGui::Button("Add Component"))
        ImGui::OpenPopup("AddComponent");

    if(ImGui::BeginPopup("AddComponent")){
        for(auto& i: SceneManager::Get()._coreComponentsSerializer){
            if(ImGui::MenuItem(i.first)){
                i.second.addComponent(_editor->_selectionEntity);
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing(); 

        for(auto& i: SceneManager::Get()._componentsSerializer){
            if(ImGui::MenuItem(i.first)){
                i.second.addComponent(_editor->_selectionEntity);
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::EndPopup();
    }
}

}