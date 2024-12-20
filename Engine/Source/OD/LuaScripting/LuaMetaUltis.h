#pragma once
#include "OD/Defines.h"
#include <entt/entt.hpp>
#include "OD/Core/Lua.h"

namespace OD{

[[nodiscard]] entt::id_type GetIdType(const sol::table& comp);

template<typename ...Args> 
inline auto InvokeMetaFunction(entt::meta_type meta, entt::id_type funcId, Args&& ...args){
    if(!meta){
        LogError("No entt::meta_type has been provided or is invalid!");
        Assert(false && "No entt::meta_type has been provided or is invalid!");
        return entt::meta_any{};
    }

    if(auto metaFunction = meta.func(funcId); metaFunction){
        return metaFunction.invoke({}, std::forward<Args>(args) ...);
    }

    return entt::meta_any{};
}

template<typename ...Args> 
inline auto InvokeMetaFunction(entt::id_type id, entt::id_type funcId, Args&& ...args){
    return InvokeMetaFunction(entt::resolve(id), funcId, std::forward<Args>(args) ...);
}

}