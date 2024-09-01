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
void _LoadComponent(ODInputArchive& archive, std::unordered_map<entt::entity,entt::entity>& loadLookup, entt::registry& registry, std::string componentName){
    std::vector<T> components;
    std::vector<entt::entity> componentsEntities;

    try{

    archive(cereal::make_nvp(componentName + "s", components));
    archive(cereal::make_nvp(componentName + "Entities", componentsEntities));

    }catch(...){ 
        LogWarning("ErrorOnTrySerialize: %s", componentName.c_str()); 
        components.clear();
        componentsEntities.clear();
    }

    for(int i = 0; i < components.size(); i++){
        //registry.get_or_emplace<T>(loadLookup[componentsEntities[i]], components[i]);

        if(registry.any_of<T>(loadLookup[componentsEntities[i]])){
            T& t = registry.get<T>(loadLookup[componentsEntities[i]]);
            t = components[i];
        } else {
            registry.emplace<T>(loadLookup[componentsEntities[i]], components[i]);
        }
    }
}

HAS_MEM_FUNC(OnGui, HasOnGui);
//HAS_TEMPLATE_FUNC(serialize, HasSerialize);

template<typename T>
void SceneManager::RegisterCoreComponent(const char* name){
    Assert(coreComponentsSerializer.find(name) == coreComponentsSerializer.end());
    //LogInfo("OnRegisterCoreComponent: %s", name.c_str());

    SerializeFuncs funcs;

    funcs.hasComponent = [](Entity& e){ return e.HasComponent<T>(); };
    funcs.addComponent = [](Entity& e){ e.AddOrGetComponent<T>(); };
    funcs.removeComponent = [](Entity& e){ e.RemoveComponent<T>(); };

    funcs.onGui = [](Entity& e){
        if constexpr(HasOnGui<T>::value){
            e.AddOrGetComponent<T>();
            T::OnGui(e);
        } else {
            T& c = e.AddOrGetComponent<T>();
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
void SceneManager::RegisterComponent(const char* name){
    Assert(componentsSerializer.find(name) == componentsSerializer.end());

    SerializeFuncs funcs;

    funcs.hasComponent = [](Entity& e){ return e.HasComponent<T>(); };
    funcs.addComponent = [](Entity& e){ e.AddOrGetComponent<T>(); };
    funcs.removeComponent = [](Entity& e){ e.RemoveComponent<T>(); };

    funcs.onGui = [](Entity& e){
        if constexpr(HasOnGui<T>::value){
            e.AddOrGetComponent<T>();
            T::OnGui(e);
        } else {
            T& c = e.AddOrGetComponent<T>();
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