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
void _SaveComponent(ODOutputArchive& archive, entt::registry& registry, std::string componentName){
    auto view = registry.view<T>();
    std::vector<T> components;
    std::vector<entt::entity> componentsEntities;
    for(auto e: view){
        components.push_back(view.template get<T>(e));
        componentsEntities.push_back(e);
    }

    if(components.size() <= 0 || componentsEntities.size() <= 0) return;

    archive(cereal::make_nvp(componentName + "s", components));
    archive(cereal::make_nvp(componentName + "Entities", componentsEntities));
}

template<typename T>
void _LoadComponent(ODInputArchive& archive, entt::registry& registry, std::string componentName){
    std::vector<T> components;
    std::vector<entt::entity> componentsEntities;

    try{

    archive(cereal::make_nvp(componentName + "s", components));
    archive(cereal::make_nvp(componentName + "Entities", componentsEntities));

    }catch(...){ 
        LogWarning("ErrorOnTrySerialize"); 
        components.clear();
        componentsEntities.clear();
    }

    for(int i = 0; i < components.size(); i++){
        registry.emplace<T>(componentsEntities[i], components[i]);
    }
}

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

    funcs.snapshotOut = [&](ODOutputArchive& out, entt::registry& registry, std::string name){
        //LogWarning("Saving Component %s", name.c_str());
        _SaveComponent<T>(out, registry, name);
    };

    funcs.snapshotIn = [](ODInputArchive& in, entt::registry& registry, std::string name){
        _LoadComponent<T>(in, registry, name);
    };
    
    coreComponentsSerializer[name] = funcs;
}

template<typename T>
void SceneManager::RegisterCoreComponentSimple(const char* name){
    //Assert(coreComponentsSerializer.find(name) == coreComponentsSerializer.end());
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

    funcs.snapshotOut = [](ODOutputArchive& out, entt::registry& registry, std::string name){
        _SaveComponent<T>(out, registry, name);
    };

    funcs.snapshotIn = [](ODInputArchive& out, entt::registry& registry, std::string name){
        _LoadComponent<T>(out, registry, name);
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

    funcs.snapshotOut = [](ODOutputArchive& out, entt::registry& registry, std::string name){
        _SaveComponent<T>(out, registry, name);
    };

    funcs.snapshotIn = [](ODInputArchive& out, entt::registry& registry, std::string name){
        _LoadComponent<T>(out, registry, name);
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