//#pragma once
//#include "Scene.h"

namespace OD{

//-----------TransformComponent---------
template <class Archive>
void TransformComponent::serialize(Archive & ar){
    ArchiveDump(ar, CEREAL_NVP(transform.localPosition)); 
    ArchiveDump(ar, CEREAL_NVP(transform.localRotation));
    ArchiveDump(ar, CEREAL_NVP(transform.localEulerAngles)); 
    ArchiveDump(ar, CEREAL_NVP(transform.localScale));
    ArchiveDump(ar, CEREAL_NVP(children));
    ArchiveDump(ar, CEREAL_NVP(parent));
    ArchiveDump(ar, CEREAL_NVP(hasParent));
}

//-----------InfoComponent---------
template <class Archive>
void InfoComponent::serialize(Archive& ar){
    ArchiveDumpNVP(ar, name);
    ArchiveDumpNVP(ar, tag);
    ArchiveDumpNVP(ar, active);
    ArchiveDumpNVP(ar, entityType);
    ArchiveDumpNVP(ar, prefabPath);
}

HAS_MEM_FUNC(OnCreate, HasOnCreate);

//-----------Entity---------
template<typename T>
T& Entity::AddComponent(){
    T& c = scene->registry.emplace<T>(id);
    if constexpr(HasOnCreate<T>::value) c.OnCreate(*this);

    return c;

    //return scene->registry.emplace<T>(id);
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

template<typename... T, typename Func> 
Entity Scene::AddEntityWith(std::string name, Func func){
    Entity e = AddEntity(name);
    (e.AddOrGetComponent<T>(), ...);
    func( std::forward<T&>(e.GetComponent<T>())... );
    return e;
}

template <typename T>
void Scene::AddSystem(){
    static_assert(std::is_base_of<OD::System, T>::value);
    Assert(systems.find(GetType<T>()) == systems.end() && "System Already has been added");

    auto newSystem = new T(this);
    //newSystem->Init(this);

    systems[GetType<T>()] = newSystem;
    systemsAdd[GetType<T>()] = [](Scene& s){ s.AddSystem<T>(); };

    if(newSystem->Type() == SystemType::Stand) standSystems.push_back(newSystem);
    if(newSystem->Type() == SystemType::Renderer) rendererSystems.push_back(newSystem);
    if(newSystem->Type() == SystemType::Physics) physicsSystems.push_back(newSystem);
}

template<typename T> 
void Scene::RemoveSystem(){
    static_assert(std::is_base_of<OD::System, T>::value);
    Assert(systems.find(GetType<T>()) != systems.end() && "System Already has not been added");

    System* s = systems[GetType<T>()];

    systems.erase(GetType<T>());
    systemsAdd.erase(GetType<T>());

    standSystems.erase(std::remove(standSystems.begin(), standSystems.end(), s), standSystems.end());
    rendererSystems.erase(std::remove(rendererSystems.begin(), rendererSystems.end(), s), rendererSystems.end());
    physicsSystems.erase(std::remove(physicsSystems.begin(), physicsSystems.end(), s), physicsSystems.end());

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