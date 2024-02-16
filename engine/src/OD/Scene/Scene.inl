//#pragma once
//#include "Scene.h"

namespace OD{

//-----------TransformComponent---------
template <class Archive>
void TransformComponent::serialize(Archive & ar){
    ar(
        CEREAL_NVP(transform.localPosition), 
        CEREAL_NVP(transform.localRotation),
        CEREAL_NVP(transform.localEulerAngles), 
        CEREAL_NVP(transform.localScale), 
        CEREAL_NVP(transform.isDirt),
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
    standSystems.erase(std::remove(standSystems.begin(), standSystems.end(), s), standSystems.end());
    rendererSystems.erase(std::remove(rendererSystems.begin(), rendererSystems.end(), s), rendererSystems.end());
    physicsSystems.erase(std::remove(physicsSystems.begin(), physicsSystems.end(), s), physicsSystems.end());
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