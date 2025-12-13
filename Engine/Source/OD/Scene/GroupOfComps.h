#pragma once
#include "Scene.h"

namespace OD{

template<typename... Components>
struct GroupOfComps { //TODO: Finish this
    static Entity Create(Registry& registry, Components&&... components){
        Entity entity = registry.create();
        (EmplaceComponent<Components>(registry, entity, std::forward<Components>(components)), ...);
        return entity;
    }

    static Entity Create(Registry& registry){
        Entity entity = registry.create();
        (EmplaceComponent<Components>(registry, entity, Components{}), ...);
        return entity;
    }

    template<typename Func>
    static Entity Create(Registry& registry, Func&& func){
        Entity entity = registry.create();
        func( EmplaceComponent<Components>(registry, entity, Components{})... );
        //(EmplaceOrGetComponent<Components>(registry, entity), ...);
        //func(GetComponent<Components>(registry, entity)...);
        return entity;
    }

    static Entity Create(Scene& scene, const std::string& name, Components&&... components){
        Entity entity = scene.AddEntity(name);
        (EmplaceComponent<Components>(scene.GetRegistry(), entity, std::forward<Components>(components)), ...);
        return entity;
    }
    
    static Entity Create(Scene& scene, const std::string& name){
        Entity entity = scene.AddEntity(name);
        (EmplaceComponent<Components>(scene.GetRegistry(), entity, Components{}), ...);
        return entity;
    }

    template<typename Func>
    static Entity Create(Scene& scene, const std::string& name, Func&& func){
        Entity entity = scene.AddEntity(name);
        //func(EmplaceComponent<Components>(scene.GetRegistry(), entity, Components{}), ...);
        (EmplaceOrGetComponent<Components>(scene.registry, entity), ...);
        func(GetComponent<Components>(scene.registry, entity)...);
        return entity;
    }

    template<typename Component>
    static Component& EmplaceComponent(Registry& registry, Entity entity, Component&& component){
        if constexpr (std::is_same_v<std::decay_t<Component>, TransformComponent>){
            std::cout << "ToImplement: Special handling for TransformComponent\n"; //TODO: Finish this
            if(!registry.any_of<TransformComponent>(entity)) return registry.emplace<TransformComponent>(entity, std::forward<Component>(component));
        } else if constexpr (std::is_same_v<std::decay_t<Component>, InfoComponent>) { 
            std::cout << "ToImplement: Special handling for InfoComponent\n"; //TODO: Finish this
            if(!registry.any_of<InfoComponent>(entity)) return registry.emplace<InfoComponent>(entity, std::forward<Component>(component));
        }
        return registry.emplace<Component>(entity, std::forward<Component>(component));
    }

    template<typename Component>
    static Component& EmplaceOrGetComponent(Registry& registry, Entity entity) {
        if (!registry.any_of<Component>(entity)) {
            return registry.emplace<Component>(entity);
        }
        return registry.get<Component>(entity);
    }

    template<typename Component>
    static Component& GetComponent(Registry& registry, Entity entity) {
        return registry.get<Component>(entity);
    }

    static auto GetView(Registry& registry){
        return registry.view<Components...>();
    }
};

}