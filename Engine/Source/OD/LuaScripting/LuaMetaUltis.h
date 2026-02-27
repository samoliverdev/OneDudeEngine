#pragma once
#include "OD/Defines.h"
#include "OD/Base.h"
#include "OD/Core/Log.h"
#include "OD/Core/Lua.h"
#define ENTT_ASSERT(condition, msg) Assert((condition) && (msg))
#include <entt/entt.hpp>

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