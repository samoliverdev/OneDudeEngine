#pragma once
#include "SceneManager.inl"
#include "Scripts.h"

namespace OD{

template<typename T>
struct CoreComponentTypeRegistrator{
    CoreComponentTypeRegistrator(const char* name){
        SceneManager::Get().RegisterCoreComponent<T>(name);
    }
};
template<typename T>
void _SaveComponent(ODOutputArchive& archive, std::vector<entt::entity>& entities, entt::registry& registry, std::string componentName){
    /*auto view = registry.view<T>();
    std::vector<T> components;
    std::vector<entt::entity> componentsEntities;
    for(auto e: view){
        components.push_back(view.template get<T>(e));
        componentsEntities.push_back(e);
    }

    if(components.size() <= 0 || componentsEntities.size() <= 0) return;

    archive(cereal::make_nvp(componentName + "s", components));
    archive(cereal::make_nvp(componentName + "Entities", componentsEntities));*/

    std::vector<T> components;
    std::vector<entt::entity> componentsEntities;
    for(auto e: entities){
        if(registry.any_of<T>(e) == false) continue;

        components.push_back(registry.get<T>(e));
        componentsEntities.push_back(e);
    }

    if(components.size() <= 0 || componentsEntities.size() <= 0) return;

    archive(cereal::make_nvp(componentName + "s", components));
    archive(cereal::make_nvp(componentName + "Entities", componentsEntities));
}
template<typename T>
void _SaveComponentTag(ODOutputArchive& archive, std::vector<entt::entity>& entities, entt::registry& registry, std::string componentName){
    /*auto view = registry.view<T>();
    std::vector<T> components;
    std::vector<entt::entity> componentsEntities;
    for(auto e: view){
        components.push_back(view.template get<T>(e));
        componentsEntities.push_back(e);
    }

    if(components.size() <= 0 || componentsEntities.size() <= 0) return;

    archive(cereal::make_nvp(componentName + "s", components));
    archive(cereal::make_nvp(componentName + "Entities", componentsEntities));*/

    std::vector<T> components;
    std::vector<entt::entity> componentsEntities;
    for(auto e: entities){
        if(registry.any_of<T>(e) == false) continue;

        components.push_back(T());
        componentsEntities.push_back(e);
    }

    if(components.size() <= 0 || componentsEntities.size() <= 0) return;

    archive(cereal::make_nvp(componentName + "s", components));
    archive(cereal::make_nvp(componentName + "Entities", componentsEntities));
}

template<typename T>
void _LoadComponent(ODInputArchive& archive, std::unordered_map<entt::entity,entt::entity>& loadLookup, entt::registry& registry, std::string componentName){
    std::vector<T> components;
    std::vector<entt::entity> componentsEntities;

    try{

    archive(cereal::make_nvp(componentName + "s", components));
    archive(cereal::make_nvp(componentName + "Entities", componentsEntities));

    }catch(...){ 
        //LogWarning("ErrorOnTrySerialize: %s", componentName.c_str()); 
        components.clear();
        componentsEntities.clear();
    }

    for(int i = 0; i < components.size(); i++){
        //registry.get_or_emplace<T>(loadLookup[componentsEntities[i]], components[i]);
        //continue;

        if(registry.any_of<T>(loadLookup[componentsEntities[i]])){
            T& t = registry.get<T>(loadLookup[componentsEntities[i]]);
            t = components[i];
        } else {
            registry.emplace<T>(loadLookup[componentsEntities[i]], components[i]);
        }
    }
}

template<typename T>
void _LoadComponent(ODInputArchive& archive, std::unordered_map<entt::entity,entt::entity>& loadLookup, entt::registry& registry, std::string componentName, std::function<void(T&)> posProcessing){
    std::vector<T> components;
    std::vector<entt::entity> componentsEntities;

    try{

    archive(cereal::make_nvp(componentName + "s", components));
    archive(cereal::make_nvp(componentName + "Entities", componentsEntities));

    }catch(...){ 
        //LogWarning("ErrorOnTrySerialize: %s", componentName.c_str()); 
        components.clear();
        componentsEntities.clear();
    }

    for(int i = 0; i < components.size(); i++){
        //registry.get_or_emplace<T>(loadLookup[componentsEntities[i]], components[i]);
        //continue;

        if(registry.any_of<T>(loadLookup[componentsEntities[i]])){
            T& t = registry.get<T>(loadLookup[componentsEntities[i]]);
            t = components[i];
        } else {
            registry.emplace<T>(loadLookup[componentsEntities[i]], components[i]);
        }
    }
}

template<typename T>
void _LoadComponentTag(ODInputArchive& archive, std::unordered_map<entt::entity,entt::entity>& loadLookup, entt::registry& registry, std::string componentName){
    std::vector<T> components;
    std::vector<entt::entity> componentsEntities;

    try{

    archive(cereal::make_nvp(componentName + "s", components));
    archive(cereal::make_nvp(componentName + "Entities", componentsEntities));

    }catch(...){ 
        //LogWarning("ErrorOnTrySerialize: %s", componentName.c_str()); 
        components.clear();
        componentsEntities.clear();
    }

    for(int i = 0; i < components.size(); i++){
        //registry.get_or_emplace<T>(loadLookup[componentsEntities[i]], components[i]);
        //continue;

        if(registry.any_of<T>(loadLookup[componentsEntities[i]])){
            T& t = T(); //registry.get<T>(loadLookup[componentsEntities[i]]);
            t = components[i];
        } else {
            registry.emplace<T>(loadLookup[componentsEntities[i]], components[i]);
        }
    }
}


HAS_MEM_FUNC(OnGui, HasOnGui);
//HAS_TEMPLATE_FUNC(serialize, HasSerialize);

template<typename T>
void SceneManager::RegisterCoreComponent(const std::string& name, const std::string& groupName){
    //Assert(coreComponentsSerializer.find(name) == coreComponentsSerializer.end());
    //LogInfo("OnRegisterCoreComponent: %s", name.c_str());

    SerializeFuncs funcs;

    funcs.groupName = groupName;
    funcs.displayName = name.substr(name.find_last_of('/') + 1);
    funcs.hasComponent = [](Entity& e, Scene& scene){ return scene.HasComponent<T>(e); };
    funcs.addComponent = [](Entity& e, Scene& scene){ scene.AddOrGetComponent<T>(e); };
    funcs.removeComponent = [](Entity& e, Scene& scene){ scene.RemoveComponent<T>(e); };
    funcs.copyComponent = [](Entity& e, Entity& other, Scene& source, Scene& target){ 
        target.GetRegistry().emplace_or_replace<T>(other, source.GetComponent<T>(e)); 
    };

    funcs.onGui = [](Entity& e, Scene& scene){
        if constexpr(HasOnGui<T>::value){
            scene.AddOrGetComponent<T>(e);
            T::OnGui(e, scene);
        } else {
            T& c = scene.AddOrGetComponent<T>(e);
            cereal::ImGuiArchive uiArchive;
            uiArchive(c);
        }
    };

    funcs.copy = [](entt::registry& dst, entt::registry& src){
        auto view = src.view<T>();
        for(auto e: view){
            T& c = view.template get<T>(e);
            dst.emplace_or_replace<T>(e, c);
        }
    };

    funcs.snapshotOut = [&](ODOutputArchive& out, std::vector<entt::entity>& entities, entt::registry& registry, std::string name){
        //LogWarning("Saving Component %s", name.c_str());
        _SaveComponent<T>(out, entities, registry, name);
    };

    funcs.snapshotIn = [](ODInputArchive& in, std::unordered_map<entt::entity,entt::entity>& loadLookup, entt::registry& registry, std::string name){
        _LoadComponent<T>(in, loadLookup, registry, name);
    };
    
    coreComponentsSerializer[name] = funcs;
}

template<typename T>
void SceneManager::UnRegisterCoreComponent(const std::string& name){
    //coreComponentsSerializer[name] = funcs;
    LogWarning("UnRegisterCoreComponent: %s", name.c_str());
    coreComponentsSerializer.erase(name); 
}

/*template<typename T>
void SceneManager::RegisterCoreComponentSimple(const char* name){
    //Assert(coreComponentsSerializer.find(name) == coreComponentsSerializer.end());
    //LogInfo("OnRegisterCoreComponent: %s", name.c_str());

    CoreComponent funcs;

    funcs.hasComponent = [](Entity& e){ return e.HasComponent<T>(); };
    funcs.addComponent = [](Entity& e){ e.AddOrGetComponent<T>(); };
    funcs.removeComponent = [](Entity& e){ e.RemoveComponent<T>(); };

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

    funcs.snapshotOut = [](ODOutputArchive& out, std::vector<entt::entity>& entities, entt::registry& registry, std::string name){
        _SaveComponent<T>(out, entities, registry, name);
    };

    funcs.snapshotIn = [](ODInputArchive& out, std::unordered_map<entt::entity,entt::entity>& loadLookup, entt::registry& registry, std::string name){
        _LoadComponent<T>(out, loadLookup, registry, name);
    };
    
    coreComponentsSerializer[name] = funcs;
}*/

template<typename T> 
void SceneManager::RegisterTagComponent(const std::string& name, const std::string& groupName){
    Assert(componentsSerializer.find(name) == componentsSerializer.end());

    SerializeFuncs funcs;

    funcs.groupName = groupName;
    funcs.displayName = name.substr(name.find_last_of('/') + 1);
    funcs.hasComponent = [](Entity& e, Scene& scene){ return scene.HasComponent<T>(e); };
    funcs.addComponent = [](Entity& e, Scene& scene){ scene.GetRegistry().emplace<T>(e); };
    funcs.removeComponent = [](Entity& e, Scene& scene){ scene.RemoveComponent<T>(e); };
    funcs.copyComponent = [](Entity& e, Entity& other, Scene& source, Scene& target){ 
        target.GetRegistry().emplace_or_replace<T>(other, T()); 
    };

    funcs.onGui = [](Entity& e, Scene& scene){
        /*if constexpr(HasOnGui<T>::value){
            scene.AddOrGetComponent<T>(e);
            T::OnGui(e, scene);
        } else {
            T& c = scene.AddOrGetComponent<T>(e);
            cereal::ImGuiArchive uiArchive;
            uiArchive(c);
        }*/
    };

    funcs.copy = [](entt::registry& dst, entt::registry& src){
        auto view = src.view<T>();
        for(auto e: view){
            dst.emplace_or_replace<T>(e, T());
        }
    };

    funcs.snapshotOut = [](ODOutputArchive& out, std::vector<entt::entity>& entities, entt::registry& registry, std::string name){
        _SaveComponentTag<T>(out, entities, registry, name);
    };

    funcs.snapshotIn = [](ODInputArchive& out, std::unordered_map<entt::entity,entt::entity>& loadLookup, entt::registry& registry, std::string name){
        _LoadComponentTag<T>(out, loadLookup, registry, name);
    };
    
    componentsSerializer[name] = funcs;
}

template<typename T>
void SceneManager::RegisterComponent(const std::string& name, const std::string& groupName){
    Assert(componentsSerializer.find(name) == componentsSerializer.end());

    SerializeFuncs funcs;

    funcs.groupName = groupName;
    funcs.displayName = name.substr(name.find_last_of('/') + 1);
    funcs.hasComponent = [](Entity& e, Scene& scene){ return scene.HasComponent<T>(e); };
    funcs.addComponent = [](Entity& e, Scene& scene){ scene.AddOrGetComponent<T>(e); };
    funcs.removeComponent = [](Entity& e, Scene& scene){ scene.RemoveComponent<T>(e); };
    funcs.copyComponent = [](Entity& e, Entity& other, Scene& source, Scene& target){ 
        target.GetRegistry().emplace_or_replace<T>(other, source.GetComponent<T>(e)); 
    };

    funcs.onGui = [](Entity& e, Scene& scene){
        if constexpr(HasOnGui<T>::value){
            scene.AddOrGetComponent<T>(e);
            T::OnGui(e, scene);
        } else {
            T& c = scene.AddOrGetComponent<T>(e);
            cereal::ImGuiArchive uiArchive;
            uiArchive(c);
        }
    };

    funcs.copy = [](entt::registry& dst, entt::registry& src){
        auto view = src.view<T>();
        for(auto e: view){
            T& c = view.template get<T>(e);
            dst.emplace_or_replace<T>(e, c);
        }
    };

    funcs.snapshotOut = [](ODOutputArchive& out, std::vector<entt::entity>& entities, entt::registry& registry, std::string name){
        _SaveComponent<T>(out, entities, registry, name);
    };

    funcs.snapshotIn = [](ODInputArchive& out, std::unordered_map<entt::entity,entt::entity>& loadLookup, entt::registry& registry, std::string name){
        _LoadComponent<T>(out, loadLookup, registry, name);
    };
    
    componentsSerializer[name] = funcs;
}

template<typename T>
void SceneManager::RegisterScript(const std::string& name){
    Assert(scriptsSerializer.find(name) == scriptsSerializer.end());

    SerializeFuncs funcs;

    funcs.hasComponent = [](Entity& e, Scene& scene){
        if(scene.HasComponent<ScriptComponent>(e) == false) return false;

        auto& c = scene.GetComponent<ScriptComponent>(e);
        return c.HasScript<T>();
    };

    funcs.onGui = [](Entity& e, Scene& scene){
        /*auto& c = e.GetComponent<ScriptComponent>();
        T* script = c.GetScript<T>();
        cereal::ImGuiArchive uiArchive;
        uiArchive(*script);*/

        if constexpr(HasOnGui<T>::value){
            T::OnGui(e, scene);
        } else {
            auto& c = scene.GetComponent<ScriptComponent>(e);
            T* script = c.GetScript<T>();
            cereal::ImGuiArchive uiArchive;
            uiArchive(*script);
        }
    };

    scriptsSerializer[name] = funcs;
}

template<typename T>
void SceneManager::RegisterSystem(const std::string& name){
    //Assert(addSystemFuncs.find(name) == addSystemFuncs.end());

    /*addSystemFuncs[name] = [&](Scene& e){
        e.AddSystem<T>();
    };*/
    addSystemFuncs.push_back([&](Scene& e){
        e.AddSystem<T>();
    });
}

template <typename T>
void SceneManager::AddGlobalSystem(){
    static_assert(std::is_base_of<OD::System, T>::value);
    Assert(globalSystems.find(GetType<T>()) == globalSystems.end() && "System Already has been added");

    auto newSystem = new T();

    globalSystems[GetType<T>()] = newSystem;
    
    if(newSystem->Type() & SystemType::Stand) globalStandSystems.push_back(newSystem);
    if(newSystem->Type() & SystemType::Animation) globalAnimationSystems.push_back(newSystem);
    
    if(newSystem->Type() & SystemType::PrePhysics) globalPrePhysicsSystems.push_back(newSystem);
    if(newSystem->Type() & SystemType::FixedPhysics) globalFixedPhysicsSystems.push_back(newSystem);
    if(newSystem->Type() & SystemType::PostPhysics) globalPostPhysicsSystems.push_back(newSystem);
    
    if(newSystem->Type() & SystemType::Late) globalLateSystems.push_back(newSystem);
    if(newSystem->Type() & SystemType::Renderer) globalRendererSystems.push_back(newSystem);
}

template<typename T> 
void SceneManager::RemoveGlobalSystem(){
    static_assert(std::is_base_of<OD::System, T>::value);
    Assert(globalSystems.find(GetType<T>()) != globalSystems.end() && "System Already has not been added");

    System* s = globalSystems[GetType<T>()];

    globalSystems.erase(GetType<T>());
    
    globalStandSystems.erase(std::remove(globalStandSystems.begin(), globalStandSystems.end(), s), globalStandSystems.end());
    globalAnimationSystems.erase(std::remove(globalAnimationSystems.begin(), globalAnimationSystems.end(), s), globalAnimationSystems.end());

    globalPrePhysicsSystems.erase(std::remove(globalPrePhysicsSystems.begin(), globalPrePhysicsSystems.end(), s), globalPrePhysicsSystems.end());
    globalFixedPhysicsSystems.erase(std::remove(globalFixedPhysicsSystems.begin(), globalFixedPhysicsSystems.end(), s), globalFixedPhysicsSystems.end());
    globalPostPhysicsSystems.erase(std::remove(globalPostPhysicsSystems.begin(), globalPostPhysicsSystems.end(), s), globalPostPhysicsSystems.end());

    globalLateSystems.erase(std::remove(globalLateSystems.begin(), globalLateSystems.end(), s), globalLateSystems.end());
    globalRendererSystems.erase(std::remove(globalRendererSystems.begin(), globalRendererSystems.end(), s), globalRendererSystems.end());
    
    delete s;
}

template<typename T>
T* SceneManager::GetGlobalSystem(){
    static_assert(std::is_base_of<OD::System, T>::value);

    if(globalSystems.find(GetType<T>()) == globalSystems.end()) return nullptr;
    return static_cast<T*>(globalSystems[GetType<T>()]);
}

template<typename T>
T* SceneManager::GetGlobalSystemDynamic(){
    static_assert(std::is_base_of<OD::System, T>::value);

    for(auto c: globalSystems){
        if(dynamic_cast<T*>(c.second)) return (T*)c.second;
    }
    return nullptr;
}

}