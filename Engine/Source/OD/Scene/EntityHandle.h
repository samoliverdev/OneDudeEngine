#pragma once
#include "Scene.h"

namespace OD{
struct OD_API EntityHandle{
public:
    friend struct Scene;

    EntityHandle() = default;
    EntityHandle(Entity _entity, Scene* _scene):entity(_entity), scene(_scene){}

    //template<typename T, typename... Args> T& AddComponent(Args&&... args);
    template<typename T> inline T& AddComponent(){ return scene->AddComponent<T>(entity); }
    template<typename T> inline T& GetComponent(){ return scene->GetComponent<T>(entity); }
    template<typename T> inline T* TryGetComponent(){ return scene->TryGetComponent<T>(entity); }
    template<typename T> inline T* TryGetComponentInParent(){ return scene->TryGetComponentInParent<T>(entity); }
    template<typename T> inline T* TryGetComponentInChildren(){ return scene->TryGetComponentInChildren<T>(entity); }
    template<typename T> inline bool HasComponent(){ return scene->HasComponent<T>(entity); }
    template<typename T> inline T& AddOrGetComponent(){ return scene->AddOrGetComponent<T>(entity); }
    template<typename T> inline void RemoveComponent(){ return scene->RemoveComponent<T>(entity); }

    inline bool IsValid(){ return scene != nullptr && scene->registry.valid(entity); }
    inline Entity GetEntity(){ return entity; }
    inline Scene* GetScene(){ return scene; }

    inline bool operator==(const EntityHandle& other) const { return entity == other.entity && scene == other.scene; }
    inline bool operator!=(const EntityHandle& other) const { return !(*this == other); }

    static void CreateLuaBind(sol::state& lua);

private:
    Entity entity = entt::null;
    Scene* scene = nullptr;
};

}