#include "LuaMetaUltis.h"

namespace OD{

entt::id_type GetIdType(const sol::table& comp){
    if(!comp.valid()){
        LogError("Failed to get the type id -- Component has not been exposed to lua!");
        Assert(comp.valid() && "Failed to get the type id -- Component has not been exposed to lua!");
        return -1;
    }

    const auto func = comp["TypeId"].get<sol::function>();
    Assert(
        func.valid() &&
        "[TypeId] - function has not been exposed to lua!"
        "\nPlease ensure all components and types have a TypeId function"
        "\nWhen creationg a new usertype"
    );

    return func.valid() ? func().get<entt::id_type>() : -1;
}

}