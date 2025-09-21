//#pragma once
//#include "Scene.h"

namespace OD{

//-----------TransformComponent---------
template<typename... Components, typename Func>
void TransformComponent::ForEachWithTransformTaskflow(Scene& scene, Func&& func){
    #ifdef ExperimentalTransformOptimzation

    auto& registry = scene.GetRegistry();
    auto& taskflow = scene.GetTaskflow();
    auto& executor = scene.GetExecutor();

    std::unordered_map<Entity, tf::Task> entityTasks;

    auto view = registry.view<TransformComponent, Components...>();

    // First pass: create tasks for each entity
    for(auto entity : view){
        tf::Task task = taskflow.emplace([&, entity](){
            auto& transform = registry.get<TransformComponent>(entity);
            if constexpr(sizeof...(Components) > 0){
                func(entity, transform, registry.get<Components>(entity)...);
            } else {
                func(entity, transform);
            }
        }).name("TransformTask");

        entityTasks[entity] = task;
    }

    // Second pass: setup dependencies (parent before child)
    for(auto [entity, task]: entityTasks){
        auto& transform = registry.get<TransformComponent>(entity);
        if(transform.HasParent()){
            TransformComponent& p = scene.GetComponent<TransformComponent>(transform.Parent());
            if(p.isCollection) continue;

            Entity parent = transform.Parent();
            if (auto it = entityTasks.find(parent); it != entityTasks.end()) {
                it->second.precede(task); // parent -> child
            }
        }
    }

    executor.run(taskflow).wait();
    taskflow.clear();

#endif
}

template <class Archive>
void TransformComponent::serialize(Archive & ar){
    ArchiveDump(ar, cereal::make_nvp("localPosition", localTransform.position)); 
    ArchiveDump(ar, cereal::make_nvp("localRotation", localTransform.rotation));
    ArchiveDump(ar, cereal::make_nvp("localScale", localTransform.scale));
    
    if constexpr (Archive::is_saving::value){
        std::vector<Entity> _children;
        for(auto i: children){
            if(registry->any_of<DontSave>(i) == false){
                _children.push_back(i);
            }
        }
        ArchiveDumpNamed(ar, "children", _children);
    } else {
        ArchiveDump(ar, CEREAL_NVP(children));
    }
    ArchiveDump(ar, CEREAL_NVP(parent));
    ArchiveDump(ar, CEREAL_NVP(hasParent));
    #ifdef ExperimentalTransformOptimzation
    ArchiveDump(ar, CEREAL_NVP(isCollection));
    #endif
}

//-----------InfoComponent---------
template <class Archive>
void InfoComponent::serialize(Archive& ar){
    ArchiveDumpNVP(ar, name);
    ArchiveDumpNVP(ar, tag);
    ArchiveDumpNVP(ar, layer);
    ArchiveDumpNVP(ar, active);
    ArchiveDumpNVP(ar, entityType);
    ArchiveDumpNVP(ar, prefabPath);
}

HAS_MEM_FUNC(OnCreate, HasOnCreate);

//-----------Entity---------

/*
template <typename T, typename... Args>
T& Entity::AddComponent(Args&&... args){
    T& c = scene->registry.emplace<T>(id, std::forward<Args>( args )...);
    if constexpr(HasOnCreate<T>::value) c.OnCreate(*this);
    return c;
}

template<typename T>
T& Entity::AddComponent(){
    T& c = scene->registry.emplace<T>(id);
    if constexpr(HasOnCreate<T>::value) c.OnCreate(*this);
    return c;
}

template <typename T>
T& Entity::GetComponent(){
    return scene->registry.get<T>(id);
}

template <typename T>
T* Entity::TryGetComponent(){
    return scene->registry.try_get<T>(id);
}

template<typename T> 
T* Entity::TryGetComponentInParent(){
    if(HasComponent<T>() == false){
        TransformComponent& t = GetComponent<TransformComponent>();
        if(t.HasParent()){
            Entity parent(t.Parent(), scene);
            return parent.TryGetComponentInParent<T>();
        }
    }

    return TryGetComponent<T>();
}
    
template<typename T>  
T* Entity::TryGetComponentInChildren(){
    return nullptr;
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
}*/

template<typename T>
auto _AddComponent(Scene* scene, Entity entity, const sol::table& comp, sol::this_state s){
    auto& component = scene->AddComponent<T>(entity, comp.valid() ? std::move(comp.as<T&&>()) : T{});
    //auto& component = entity.AddComponent<T>();
    return sol::make_reference(s, std::ref(component));
}

template<typename T>
bool _HasComponent(Scene* scene, Entity entity){
    return scene->HasComponent<T>(entity);
}

template<typename T>
auto _GetComponent(Scene* scene, Entity entity, sol::this_state s){
    auto& comp = scene->GetComponent<T>(entity);
    return sol::make_reference(s, std::ref(comp));
}

template<typename T>
void _RemoveComponent(Scene* scene, Entity entity){
    scene->RemoveComponent<T>(entity);
}

template<typename T>
inline void Scene::RegisterMetaComponent(){
    using namespace entt::literals;
    entt::meta<T>()
        .type(entt::type_hash<T>::value())
        .template func<&_AddComponent<T>>("_AddComponent"_hs)
        .template func<&_HasComponent<T>>("_HasComponent"_hs)
        .template func<&_GetComponent<T>>("_GetComponent"_hs)
        .template func<&_RemoveComponent<T>>("_RemoveComponent"_hs);
}

//-----------Scene---------

/*template<typename... T, typename Func> 
Entity Scene::AddEntityWith(std::string name, Func func){
    Entity e = AddEntity(name);
    (e.AddOrGetComponent<T>(), ...);
    func( std::forward<T&>(e.GetComponent<T>())... );
    return e;
}*/

template<typename... T, typename Func> 
Entity Scene::AddEntityWith(std::string name, Func func){
    Entity e = AddEntity(name);
    (AddOrGetComponent<T>(e), ...);
    func( std::forward<T&>(GetComponent<T>(e))... );
    return e;
}

template <typename T, typename... Args>
T& Scene::AddComponent(Entity id, Args&&... args){
    T& c = registry.emplace<T>(id, std::forward<Args>( args )...);
    if constexpr(HasOnCreate<T>::value) c.OnCreate(*this);
    return c;
}

template<typename T>
T& Scene::AddComponent(Entity id){
    T& c = registry.emplace<T>(id);
    if constexpr(HasOnCreate<T>::value) c.OnCreate(id, *this);
    return c;
}

template<typename T> 
void Scene::AddTagComponent(Entity entity){
    registry.emplace<T>(entity);
}

template <typename T>
T& Scene::GetComponent(Entity id){
    //if(registry.any_of<T>(id) == false) throw 0;
    return registry.get<T>(id);
}

template<typename T> 
void Scene::AddComponentRecursive(Entity entity){
    if(registry.any_of<T>(entity) == false){
        registry.emplace<T>(entity);
        if constexpr(HasOnCreate<T>::value) c.OnCreate(entity, *this);
    }

    TransformComponent& trans = registry.get<TransformComponent>(entity);
    for(auto& child: trans.children){
        AddComponentRecursive<T>(child);
    }
}

template<typename T> 
void Scene::RemoveComponentRecursive(Entity entity){
    if(registry.any_of<T>(entity) == true){
        registry.remove<T>(entity);
    }

    TransformComponent& trans = registry.get<TransformComponent>(entity);
    for(auto& child: trans.children){
        RemoveComponentRecursive<T>(child);
    }
}

template <typename T>
T* Scene::TryGetComponent(Entity id){
    return registry.try_get<T>(id);
}

template<typename T> 
T* Scene::TryGetComponentInParent(Entity id){
    if(HasComponent<T>(id) == false){
        TransformComponent& t = GetComponent<TransformComponent>(id);
        if(t.HasParent()){
            return TryGetComponentInParent<T>(t.Parent());
        }
    }

    return TryGetComponent<T>(id);
}
    
template<typename T>  
T* Scene::TryGetComponentInChildren(Entity id){
    return nullptr;
}

template<typename T>
bool Scene::HasComponent(Entity id){
    return registry.any_of<T>(id);
}

template<typename T>
T& Scene::AddOrGetComponent(Entity id){
    if(HasComponent<T>(id) == false) return AddComponent<T>(id);
    return GetComponent<T>(id);
}

template<typename T>
void Scene::RemoveComponent(Entity id){
    Assert(HasComponent<T>(id) && "Entity does not have component!");
    registry.remove<T>(id);
}

template<typename T> 
Entity Scene::TryFindEntityWithComponentInParent(Entity id){
    /*if(HasComponent<T>(id) == false){
        TransformComponent& t = GetComponent<TransformComponent>(id);
        if(t.HasParent()){
            return TryFindEntityWithComponentInParent<T>(t.Parent());
        }
    }
    if(HasComponent<T>(id)) return id;*/

    if(HasComponent<T>(id)){ 
        return id;
    } else {
        TransformComponent& t = GetComponent<TransformComponent>(id);
        if(t.HasParent()){
            return TryFindEntityWithComponentInParent<T>(t.Parent());
        }
    }

    return EntityNull;
}

template <typename T>
void Scene::AddSystem(){
    static_assert(std::is_base_of<OD::System, T>::value);
    Assert(systems.find(GetType<T>()) == systems.end() && "System Already has been added");

    auto newSystem = new T(this);
    //newSystem->Init(this);

    systems[GetType<T>()] = newSystem;
    //systemsAdd[GetType<T>()] = [](Scene& s){ s.AddSystem<T>(); };
    systemsAdd.push_back([](Scene& s){ s.AddSystem<T>(); });

    /*if(newSystem->Type() == SystemType::Physics) physicsSystems.push_back(newSystem);
    if(newSystem->Type() == SystemType::Stand) standSystems.push_back(newSystem);
    if(newSystem->Type() == SystemType::Late) lateSystems.push_back(newSystem);
    if(newSystem->Type() == SystemType::Renderer) rendererSystems.push_back(newSystem);*/

    if(newSystem->Type() & SystemType::Physics) physicsSystems.push_back(newSystem);
    if(newSystem->Type() & SystemType::Stand) standSystems.push_back(newSystem);
    if(newSystem->Type() & SystemType::Animation) animationSystems.push_back(newSystem);
    if(newSystem->Type() & SystemType::Late) lateSystems.push_back(newSystem);
    if(newSystem->Type() & SystemType::Renderer) rendererSystems.push_back(newSystem);
}

template<typename T> 
void Scene::RemoveSystem(){
    static_assert(std::is_base_of<OD::System, T>::value);
    Assert(systems.find(GetType<T>()) != systems.end() && "System Already has not been added");

    System* s = systems[GetType<T>()];

    systems.erase(GetType<T>());
    systemsAdd.erase(GetType<T>());

    physicsSystems.erase(std::remove(physicsSystems.begin(), physicsSystems.end(), s), physicsSystems.end());
    standSystems.erase(std::remove(standSystems.begin(), standSystems.end(), s), standSystems.end());
    animationSystems.erase(std::remove(animationSystems.begin(), animationSystems.end(), s), animationSystems.end());
    lateSystems.erase(std::remove(lateSystems.begin(), lateSystems.end(), s), lateSystems.end());
    rendererSystems.erase(std::remove(rendererSystems.begin(), rendererSystems.end(), s), rendererSystems.end());
    
    delete s;
}

template<typename T>
T* Scene::GetSystem(){
    static_assert(std::is_base_of<OD::System, T>::value);

    if(systems.find(GetType<T>()) == systems.end()) return nullptr;
    return static_cast<T*>(systems[GetType<T>()]);
}

template<typename T>
T* Scene::GetSystemDynamic(){
    static_assert(std::is_base_of<OD::System, T>::value);

    for(auto c: systems){
        if(dynamic_cast<T*>(c.second)) return (T*)c.second;
    }
    return nullptr;
}

}