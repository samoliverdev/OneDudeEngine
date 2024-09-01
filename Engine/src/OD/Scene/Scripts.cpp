#include "Scripts.h"
#include "SceneManager.h"
#include <functional>
#include <string>

namespace OD{

void ScriptModuleInit(){
    SceneManager::Get().RegisterCoreComponent<ScriptComponent>("ScriptComponent");
    SceneManager::Get().RegisterSystem<ScriptSystem>("ScriptSystem");
}

void ScriptComponent::OnGui(Entity& e){
    const ImGuiTreeNodeFlags treeNodeFlags = 
        ImGuiTreeNodeFlags_DefaultOpen 
        | ImGuiTreeNodeFlags_Framed 
        | ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding;

    
    std::hash<std::string> hasher;
    ScriptComponent& script = e.GetComponent<ScriptComponent>();

    for(auto i: SceneManager::Get().scriptsSerializer){
        if(i.second.hasComponent(e) == false) continue;

        bool open = ImGui::TreeNodeEx((void*)hasher(i.first), treeNodeFlags, i.first);
        if(open){
            i.second.onGui(e);
        }
        ImGui::TreePop();
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
    
void ScriptComponent::_Update(Entity e){
    for(auto i: instances){
        if(i.second.instance == nullptr){
            i.second.instance = i.second.InstantiateScript(i.second);
        }

        i.second.instance->entity = e;
        Assert(i.second.instance->entity.IsValid() == true);

        if(i.second.instance->hasStarted == false){
            i.second.instance->OnStart();
            i.second.instance->hasStarted = true;
        }
        i.second.instance->OnUpdate();
    }
}

//////////////////////////////////////

ScriptSystem::ScriptSystem(Scene* inScene):System(inScene){
    this->scene->GetRegistry().on_destroy<ScriptComponent>().connect<&OnDestroyScript>();
}

ScriptSystem::~ScriptSystem(){
    this->scene->GetRegistry().on_destroy<ScriptComponent>().disconnect<&OnDestroyScript>();
}

void ScriptSystem::Update(){
    auto view = GetScene()->GetRegistry().view<ScriptComponent>();

    for(auto entity: view){
        auto& c = view.get<ScriptComponent>(entity);
        c._Update(Entity(entity, GetScene()));
    }
}

void ScriptSystem::OnDestroyScript(entt::registry & r, entt::entity e){
    ScriptComponent& s = r.get<ScriptComponent>(e);
    s.RemoveAllScripts();
    LogInfo("On Destry ScriptComponent ___");
}

}