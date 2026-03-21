#include "Scene.h"
#include "OD/Core/Lua.h"
#include <entt/meta/meta.hpp>
#include <entt/meta/factory.hpp>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_info.hpp>

namespace OD{

template<typename T>
auto _AddComponent(Scene* scene, Entity entity, const sol::table& comp, sol::this_state s);

template<typename T>
bool _HasComponent(Scene* scene, Entity entity);

template<typename T>
auto _GetComponent(Scene* scene, Entity entity, sol::this_state s);

template<typename T>
void _RemoveComponent(Scene* scene, Entity entity);

class SceneMeta{
public:
    template<typename T>
    inline static void RegisterMetaComponent();
};

}

//.inl
namespace OD {

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
inline void SceneMeta::RegisterMetaComponent(){
    using namespace entt::literals;
    entt::meta<T>()
        .type(entt::type_hash<T>::value())
        .template func<&_AddComponent<T>>("_AddComponent"_hs)
        .template func<&_HasComponent<T>>("_HasComponent"_hs)
        .template func<&_GetComponent<T>>("_GetComponent"_hs)
        .template func<&_RemoveComponent<T>>("_RemoveComponent"_hs);
}

}