#pragma once
#include "OD/Core/Lua.h"
#include "OD/Defines.h"
#include "OD/Scene/Scene.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"
#include <stdlib.h>

namespace OD{

struct OD_API LuaScriptComponent{
    friend struct LuaScriptSystem;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive & ar){}

    static void OnGui(Entity& e);

    std::string scriptPath;
private:
    sol::protected_function OnStart;
    sol::protected_function OnDestroy;
    sol::protected_function OnUpdate;
    bool hasInited = false;
    bool hasStarted = false;
};

struct OD_API LuaScriptSystem: public System{
    LuaScriptSystem(Scene* scene);
    virtual void Update() override;

private:
    Ref<sol::state> lua;

    static void RegisterLuaBindings(sol::state& lua, Scene* scene);
    static void RegisterMathLuaBindings(sol::state& lua);
};

void LuaScriptModuleInit();

}