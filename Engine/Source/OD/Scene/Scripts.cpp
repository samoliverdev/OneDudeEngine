#include "OD/pch.h"
#include "Scripts.h"
#include "SceneManager.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Core/ImGui.h"
#include <stdlib.h>

namespace OD{

void ScriptModuleInit(){
    SceneManager::Get().RegisterCoreComponent<ScriptComponent>("ScriptComponent", "Script");
    SceneManager::Get().RegisterSystem<ScriptSystem>("ScriptSystem");
}

void ScriptComponent::OnGui(Entity& e, Scene& scene){
    const ImGuiTreeNodeFlags treeNodeFlags = 
        ImGuiTreeNodeFlags_DefaultOpen 
        | ImGuiTreeNodeFlags_Framed 
        | ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding;

    
    std::hash<std::string> hasher;
    ScriptComponent& script = scene.GetComponent<ScriptComponent>(e);

    bool removeScript = false;

    for(auto i: SceneManager::Get().scriptsSerializer){
        if(i.second.hasComponent(e, scene) == false) continue;

        bool open = ImGui::TreeNodeEx((void*)hasher(i.first.c_str()), treeNodeFlags, i.first.c_str());

        if(ImGui::BeginPopupContextItem()){
            if(ImGui::MenuItem("Remove Component")){
                removeScript = true;
            }
            ImGui::EndPopup();
        }

        if(open){
            i.second.onGui(e, scene);
            ImGui::TreePop();
        }

        if(removeScript){
            i.second.removeScript(script);
            break;
        }
    }

    if(ImGui::Button("Add Script")){
        ImGui::OpenPopup("AddScript");
    }

    if(ImGui::BeginPopup("AddScript")){
        for(auto& i: SceneManager::Get().scriptsSerializer){
            if(ImGui::MenuItem(i.first.c_str())){
                i.second.addScript(script);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
}

ScriptComponent::ScriptComponent(const ScriptComponent& s){
    //LogInfo("Copping");
    for(auto i: s.instances){
        ScriptHolder holder = {i.second.InstantiateScript(i.second), i.second.InstantiateScript};
        instances[i.first] = holder;
    }
}

void ScriptComponent::RemoveAllScripts(){
    /*for(auto it = _instances.begin(); it != _instances.end();) {
        Assert(it->second.instance != nullptr);
        it->second.instance->OnDestroy();
        delete it->second.instance;
        it = _instances.erase(it);
    }*/

    for(auto i: instances){
        delete i.second.instance;
    }
    instances.clear();
}
    
void ScriptComponent::_Update(Entity e, Scene& scene, bool isLate){
    for(auto i: instances){
        if(i.second.instance == nullptr){
            i.second.instance = i.second.InstantiateScript(i.second);
        }

        i.second.instance->entity = e;
        i.second.instance->scene = &scene;
        Assert(scene.IsValid(i.second.instance->entity) == true);

        if(i.second.instance->hasStarted == false){
            i.second.instance->OnStart();
            i.second.instance->hasStarted = true;
        }
        if(isLate){
            i.second.instance->OnLateUpdate();
        } else {
            i.second.instance->OnUpdate();
        }
    }
}

void ScriptComponent::_ParallelUpdate(Entity e, Scene& scene, bool isLate){
    for(auto i: instances){
        if(i.second.instance == nullptr) return;

        i.second.instance->entity = e;
        i.second.instance->scene = &scene;
        Assert(scene.IsValid(i.second.instance->entity) == true);

        if(i.second.instance->hasStarted == false){
            i.second.instance->OnStart();
            i.second.instance->hasStarted = true;
        }
        //if(isLate){
        //    i.second.instance->OnLateUpdate();
        //} else {
            i.second.instance->OnParallelUpdate();
        //}
    }
}

//////////////////////////////////////

void ScriptSystem::OnInit(Scene& scene){
    scene.GetRegistry().on_destroy<ScriptComponent>().connect<&OnDestroyScript>();
}

void ScriptSystem::OnEnd(Scene& scene){
    scene.GetRegistry().on_destroy<ScriptComponent>().disconnect<&OnDestroyScript>();
}

void ScriptSystem::Update(Scene& scene){
    {
    OD_PROFILE_SCOPE("ScriptSystem::Update::Sync");
    scene.RunAllTaskAndSync();
    }

    OD_PROFILE_SCOPE("ScriptSystem::Update");

    auto view = scene.GetRegistry().view<ScriptComponent>();
    for(auto entity: view){
        auto& c = view.get<ScriptComponent>(entity);
        c._Update(entity, scene, false);
    }

    //INFO: Experimental
    /*GetScene()->GetTaskflow().emplace([=](tf::Subflow& subflow){
        for(auto entity: view){
            subflow.emplace([&](){ 
                auto& c = view.get<ScriptComponent>(entity);
                c._ParallelUpdate(entity, *GetScene(), false);
            });
        }
    });*/
}

void ScriptSystem::LateUpdate(Scene& scene){
    {
    OD_PROFILE_SCOPE("ScriptSystem::LateUpdate::Sync");
    scene.RunAllTaskAndSync();
    }

    OD_PROFILE_SCOPE("ScriptSystem::LateUpdate");
    auto view = scene.GetRegistry().view<ScriptComponent>();
    for(auto entity: view){
        auto& c = view.get<ScriptComponent>(entity);
        c._Update(entity, scene, true);
    }
}

void ScriptSystem::OnDestroyScript(entt::registry & r, entt::entity e){
    ScriptComponent& s = r.get<ScriptComponent>(e);
    s.RemoveAllScripts();
    //LogInfo("On Destry ScriptComponent ___");
}

}