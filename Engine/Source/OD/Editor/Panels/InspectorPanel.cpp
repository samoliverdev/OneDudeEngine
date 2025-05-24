#include "InspectorPanel.h"
#include "OD/Editor/Editor.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/Undo.h"
#include "OD/Scene/Scripts.h"
#include "OD/RenderPipeline/CameraComponent.h"
#include "OD/RenderPipeline/LightComponent.h"
#include "OD/RenderPipeline/MeshRendererComponent.h"
#include "OD/RenderPipeline/EnvironmentComponent.h"
#include "OD/Physics/PhysicsSystem.h"
//#include "OD/AnimationSystem/Animator.h"
#include <glm/gtc/type_ptr.hpp>
#include <functional>
#include <string>

namespace OD{

InspectorPanel::InspectorPanel(){
    name = "InspectorPanel";
    show = true;
}

void InspectorPanel::OnGui(){
    if(scene == nullptr) return;

    //if(ImGui::Begin("Inspector")){
    ImGui::Begin("Inspector");
    if(scene->IsValid(editor->selectionEntity) && editor->selectionOnAsset == false){
        DrawComponents(editor->selectionEntity);
        ImGui::Separator();
        ImGui::Spacing();
        ShowAddComponent(editor->selectionEntity);
    } else if(editor->selectionOnAsset == true && editor->selectionAsset != nullptr){
        editor->selectionAsset->OnGui();
    }
    ImGui::End();
}

/*template<typename T>
void DrawComponent(Entity e, const char* name){
    const ImGuiTreeNodeFlags treeNodeFlags = 
        ImGuiTreeNodeFlags_DefaultOpen 
        | ImGuiTreeNodeFlags_Framed 
        | ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding;

    if(scene->HasComponent<T>(e)){
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
            T::OnGui(e, *scene);
            ImGui::TreePop();
        }

        if(removeComponent){
            scene->RemoveComponent<T>(e);
        }
    }
}*/

template<typename T, typename UIFunction>
void DrawComponent(Entity e, Scene& scene, const char* name, UIFunction function){
    const ImGuiTreeNodeFlags treeNodeFlags = 
        ImGuiTreeNodeFlags_DefaultOpen 
        | ImGuiTreeNodeFlags_Framed 
        | ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding;

    if(scene.HasComponent<T>(e)){
        bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name);

        if(open){
            function(e, scene);
            ImGui::TreePop();
        }
    }
}

void InspectorPanel::DrawComponentFromCoreComponents(Entity e, std::string name, SceneManager::SerializeFuncs &f){
    std::hash<std::string> hasher;

    const ImGuiTreeNodeFlags treeNodeFlags = 
        ImGuiTreeNodeFlags_DefaultOpen 
        | ImGuiTreeNodeFlags_Framed 
        | ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding;

    if(f.hasComponent(e, *scene)){
        bool removeComponent = false;

        bool open = ImGui::TreeNodeEx((void*)hasher(name), treeNodeFlags, name.c_str());

        if(ImGui::BeginPopupContextItem()){
            if(ImGui::MenuItem("Remove Component")){
                removeComponent = true;
            }
            ImGui::EndPopup();
        }

        if(open){
            auto entityType = scene->GetComponent<InfoComponent>(e).Type();
            if(entityType != EntityType::Stand) ImGui::BeginDisabled(true);
            f.onGui(e, *scene);
            if(entityType != EntityType::Stand) ImGui::EndDisabled();

            ImGui::TreePop();
        }

        if(removeComponent){
            //e.RemoveComponent<T>();
            f.removeComponent(e, *scene);
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

    if(sf.hasComponent(e, *scene)){
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
            auto entityType = scene->GetComponent<InfoComponent>(e).Type();
            if(entityType != EntityType::Stand) ImGui::BeginDisabled(true);
            sf.onGui(e, *scene);
            if(entityType != EntityType::Stand) ImGui::EndDisabled();

            ImGui::TreePop();
        }

        if(removeComponent){
            //e.RemoveComponent<T>();
            sf.removeComponent(e, *scene);
        }
    }
}

#ifdef _WIN32
#define _strcpy(a, b, c) strcpy_s(a, b, c)
#else 
#define _strcpy(a, b, c) strcpy(a, c)
#endif

void InspectorPanel::DrawComponents(Entity entity){
    TransformComponent& transform = scene->GetComponent<TransformComponent>(entity);
    InfoComponent& info = scene->GetComponent<InfoComponent>(entity);
    EntityType entityType = scene->GetComponent<InfoComponent>(entity).Type();

    DrawComponent<InfoComponent>(entity, *scene, "Info", [&](Entity e, Scene& scene){
        if(entityType != EntityType::Stand) ImGui::BeginDisabled(true);
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));

        auto old = info;

        bool selfEnable = scene.HasComponent<SelfDisable>(e) == false;

        if(ImGui::Checkbox("Enable", &selfEnable)){
            if(selfEnable){
                scene.RemoveComponentRecursive<SelfDisable>(e);
            } else {
                scene.AddComponentRecursive<SelfDisable>(e);
            }
        }
        
        //ImGui::SameLine();
        
        _strcpy(buffer, sizeof(buffer), info.name.c_str());
        if(ImGui::InputText("Name", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)){
            info.name = std::string(buffer);
            UndoManager::Get().Execute(CreateScope<UndoValueComponentCommand<InfoComponent>>(&scene, e, old, info));
        }

        /*bool textEdited = false;
        if (ImGui::InputText("Name", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
            textEdited = true;  // User pressed Enter after editing
        } else if (ImGui::IsItemDeactivatedAfterEdit()) {
            textEdited = true;  // User finished editing by losing focus (clicked away)
        }
        if (textEdited) {
            info.name = std::string(buffer);
            UndoManager::Get().Execute(CreateScope<UndoValueComponentCommand<InfoComponent>>(&scene, e, old, info));
        }*/

        _strcpy(buffer, sizeof(buffer), info.tag.c_str());
        if(ImGui::InputText("Tag", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)){
            info.tag = std::string(buffer);
            UndoManager::Get().Execute(CreateScope<UndoValueComponentCommand<InfoComponent>>(&scene, e, old, info));
        }

        ImGui::Text("Id: %zd", (size_t)e);
        //ImGui::Text("Type: %d", info.Type());

        if(entityType != EntityType::Stand) ImGui::EndDisabled();
    });

    DrawComponent<TransformComponent>(entity, *scene, "Transform", [&](Entity e, Scene& scene){
        bool beginDisable = entityType == EntityType::PrefabChild;
        if(entityType == EntityType::PrefabRoot && transform.HasParent() && scene.GetComponent<InfoComponent>(transform.Parent()).Type() == EntityType::PrefabChild)
            beginDisable = true;

        if(beginDisable) ImGui::BeginDisabled(true);
        
        /*if(scene.HasComponent<RigidbodyComponent>(e)){
            RigidbodyComponent& rb = scene.GetComponent<RigidbodyComponent>(e);
            float p[] = {rb.Position().x, rb.Position().y, rb.Position().z};
            if(ImGui::DragFloat3("Position", p, 0.5f)){
                rb.Position(Vector3(p[0], p[1], p[2]));
            }
        } else {*/
            float p[] = {transform.LocalPosition().x, transform.LocalPosition().y, transform.LocalPosition().z};
            if(ImGui::DragFloat3("Position", p, 0.5f, 0, 0, "%.4f")){
                transform.LocalPosition(Vector3(p[0], p[1], p[2]));
            }
        //}  

        float r[] = {transform.LocalEulerAngles().x, transform.LocalEulerAngles().y, transform.LocalEulerAngles().z};
        if(ImGui::DragFloat3("Rotation", r, 0.5f, 0, 0, "%.4f")){
            transform.LocalEulerAngles(Vector3(r[0], r[1], r[2]));
        }  

        float s[] = {transform.LocalScale().x, transform.LocalScale().y, transform.LocalScale().z};
        if(ImGui::DragFloat3("Scale", s, 0.5f, 0, 0, "%.4f")){
            transform.LocalScale(Vector3(s[0], s[1], s[2]));
        } 
        if(beginDisable) ImGui::EndDisabled();
    });

    ImGui::Spacing();
    //ImGui::Separator();
    ImGui::Spacing();

    //if(entityType != EntityType::Stand) ImGui::BeginDisabled(true);
    
    for(auto& i: SceneManager::Get().coreComponentsSerializer){
        //LogInfo("%s", i.first.c_str());
        DrawComponentFromCoreComponents(entity, i.first, i.second);
    }

    ImGui::Spacing();
    //ImGui::Separator();
    ImGui::Spacing(); 

    for(auto& i: SceneManager::Get().componentsSerializer){
        DrawComponentFromSerializeFuncs(entity, i.first, i.second);
    }

    //if(entityType != EntityType::Stand) ImGui::EndDisabled();
}

void InspectorPanel::ShowAddComponent(Entity entity){
    if(ImGui::Button("Add Component"))
        ImGui::OpenPopup("AddComponent");

    if(ImGui::BeginPopup("AddComponent")){
        for(auto& i: SceneManager::Get().coreComponentsSerializer){
            if(ImGui::MenuItem(i.first.c_str())){
                i.second.addComponent(editor->selectionEntity, *scene);
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing(); 

        for(auto& i: SceneManager::Get().componentsSerializer){
            if(ImGui::MenuItem(i.first.c_str())){
                i.second.addComponent(editor->selectionEntity, *scene);
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::EndPopup();
    }
}

}