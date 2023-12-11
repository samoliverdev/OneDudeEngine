#pragma once
#include "Scene.h"

namespace OD{

//-----------TransformComponent---------
template <class Archive>
void TransformComponent::serialize(Archive & ar){
    ar(
        CEREAL_NVP(localPosition), 
        CEREAL_NVP(localRotation), 
        CEREAL_NVP(localScale), 
        CEREAL_NVP(isDirt),
        CEREAL_NVP(children),
        CEREAL_NVP(parent),
        CEREAL_NVP(hasParent)
    );
}

//-----------InfoComponent---------
template <class Archive>
void InfoComponent::serialize(Archive & ar){
    ar(CEREAL_NVP(name), CEREAL_NVP(tag), CEREAL_NVP(active));
}

//-----------Entity---------
template<typename T>
T& Entity::AddComponent(){
    return scene->registry.emplace<T>(id);
}

template <typename T>
T& Entity::GetComponent(){
    return scene->registry.get<T>(id);
}

template<typename T>
bool Entity::HasComponent(){
    return scene->registry.any_of<T>(id);
}

template<typename T>
T& Entity::AddOrGetComponent(){
    if(HasComponent<T>() == false) return AddComponent<T>();
    return GetComponent<T>();
}

template<typename T>
void Entity::RemoveComponent(){
    Assert(HasComponent<T>() && "Entity does not have component!");
    scene->registry.remove<T>(id);
}

//-----------Scene---------

template <typename T>
void Scene::AddSystem(){
    static_assert(std::is_base_of<OD::System, T>::value);
    Assert(systems.find(GetType<T>()) == systems.end() && "System Already has been added");

    auto newSystem = new T();
    newSystem->Init(this);

    systems[GetType<T>()] = newSystem;
    if(newSystem->Type() == SystemType::Stand) standSystems.push_back(newSystem);
    if(newSystem->Type() == SystemType::Renderer) rendererSystems.push_back(newSystem);
    if(newSystem->Type() == SystemType::Physics) physicsSystems.push_back(newSystem);
}

template<typename T>
T* Scene::GetSystem(){
    static_assert(std::is_base_of<OD::System, T>::value);

    if(systems.find(GetType<T>()) == systems.end()) return nullptr;
    return static_cast<T*>(systems[GetType<T>()]);
}

//-----------SceneManager---------
template<typename T>
void SceneManager::RegisterCoreComponent(const char* name){
    Assert(coreComponentsSerializer.find(name) == coreComponentsSerializer.end());
    //LogInfo("OnRegisterCoreComponent: %s", name.c_str());

    CoreComponent funcs;

    funcs.hasComponent = [](Entity& e){
        return e.HasComponent<T>();
    };

    funcs.addComponent = [](Entity& e){
        e.AddOrGetComponent<T>();
    };

    funcs.removeComponent = [](Entity& e){
        e.RemoveComponent<T>();
    };

    funcs.onGui = [](Entity& e){
        e.AddOrGetComponent<T>();
        T::OnGui(e);
    };

    funcs.copy = [](entt::registry& dst, entt::registry& src){
        auto view = src.view<T>();
        for(auto e: view){
            T& c = view.template get<T>(e);
            dst.emplace_or_replace<T>(e, c);
        }
    };

    funcs.snapshotOut = [](entt::snapshot& s, cereal::JSONOutputArchive& out){
        s.get<T>(out);
    };

    funcs.snapshotIn = [](entt::snapshot_loader& s, cereal::JSONInputArchive& in){
        s.get<T>(in);
    };
    
    coreComponentsSerializer[name] = funcs;
}

template<typename T>
void SceneManager::RegisterCoreComponentSimple(const char* name){
    Assert(coreComponentsSerializer.find(name) == coreComponentsSerializer.end());
    //LogInfo("OnRegisterCoreComponent: %s", name.c_str());

    CoreComponent funcs;

    funcs.hasComponent = [](Entity& e){
        return e.HasComponent<T>();
    };

    funcs.addComponent = [](Entity& e){
        e.AddOrGetComponent<T>();
    };

    funcs.removeComponent = [](Entity& e){
        e.RemoveComponent<T>();
    };

    funcs.onGui = [](Entity& e){
        T& c = e.AddOrGetComponent<T>();

        cereal::ImGuiArchive uiArchive;
        uiArchive(c);
    };

    funcs.copy = [](entt::registry& dst, entt::registry& src){
        auto view = src.view<T>();
        for(auto e: view){
            T& c = view.template get<T>(e);
            dst.emplace_or_replace<T>(e, c);
        }
    };

    funcs.snapshotOut = [](entt::snapshot& s, cereal::JSONOutputArchive& out){
        s.get<T>(out);
    };

    funcs.snapshotIn = [](entt::snapshot_loader& s, cereal::JSONInputArchive& out){
        s.get<T>(out);
    };
    
    coreComponentsSerializer[name] = funcs;
}

template<typename T>
void SceneManager::RegisterComponent(const char* name){
    Assert(componentsSerializer.find(name) == componentsSerializer.end());

    SerializeFuncs funcs;

    funcs.hasComponent = [](Entity& e){
        return e.HasComponent<T>();
    };

    funcs.addComponent = [](Entity& e){
        e.AddOrGetComponent<T>();
    };

    funcs.removeComponent = [](Entity& e){
        e.RemoveComponent<T>();
    };

    funcs.copy = [](entt::registry& dst, entt::registry& src){
        auto view = src.view<T>();
        for(auto e: view){
            T& c = view.template get<T>(e);
            dst.emplace_or_replace<T>(e, c);
        }
    };

    funcs.snapshotOut = [](entt::snapshot& s, cereal::JSONOutputArchive& out){
        s.get<T>(out);
    };

    funcs.snapshotIn = [](entt::snapshot_loader& s, cereal::JSONInputArchive& out){
        s.get<T>(out);
    };
    
    componentsSerializer[name] = funcs;
}

template<typename T>
void SceneManager::RegisterScript(const char* name){
    Assert(scriptsSerializer.find(name) == scriptsSerializer.end());

    SerializeFuncs funcs;

    funcs.hasComponent = [](Entity& e){
        if(e.HasComponent<ScriptComponent>() == false) return false;

        auto& c = e.GetComponent<ScriptComponent>();
        return c.HasScript<T>();
    };

    funcs.onGui = [](Entity& e){
        auto& c = e.GetComponent<ScriptComponent>();
        T* script = c.GetScript<T>();
        
        cereal::ImGuiArchive uiArchive;
        uiArchive(*script);
    };

    scriptsSerializer[name] = funcs;
}

template<typename T>
void SceneManager::RegisterSystem(const char* name){
    Assert(addSystemFuncs.find(name) == addSystemFuncs.end());

    addSystemFuncs[name] = [&](Scene& e){
        e.AddSystem<T>();
    };
}

}