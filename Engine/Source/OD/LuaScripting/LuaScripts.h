#pragma once
#include "OD/Defines.h"
#include "OD/Core/Lua.h"
#include "OD/Scene/Scene.h"
#include "OD/Serialization/Serialization.h"

namespace OD{

using LuaTable = std::map<std::string, struct LuaValue>;

struct LuaValue : std::variant<
    int, float, bool, std::string, LuaTable
> {
    using Base = std::variant<int, float, bool, std::string, LuaTable>;
    using Base::Base;

    template <class Archive>
    void serialize(Archive& ar) {
        std::visit([&](auto& val) {
            ar(val);
        }, *this);
    }
};

struct OD_API LuaScriptComponent{
    friend struct LuaScriptSystem;

    LuaScriptComponent() = default;
    ~LuaScriptComponent() = default;

    LuaScriptComponent(const LuaScriptComponent& other);
    LuaScriptComponent(LuaScriptComponent&& other);
    LuaScriptComponent& operator=(const LuaScriptComponent& other);
    LuaScriptComponent& operator=(LuaScriptComponent&& other);

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, scriptPath);
        if(data.valid() == true) saveData = convertSolObject(data);
        ArchiveDumpNVP(ar, saveData);
    }

    static void OnGui(Entity& e, Scene& scene);

    std::string scriptPath;
private:
    sol::protected_function OnStart;
    sol::protected_function OnDestroy;
    sol::protected_function OnUpdate;
    sol::table data;
    bool hasInited = false;
    bool hasStarted = false;

    LuaValue saveData = LuaTable{};
};

struct OD_API LuaScriptSystem: public System{
    LuaScriptSystem(Scene* scene);
    ~LuaScriptSystem();

    virtual void Update() override;
    inline bool ExecuteAlways() override { return true; }

private:
    Ref<sol::state> lua;
    static void OnDestroyScript(entt::registry & r, entt::entity e);
};

class OD_API LuaModule: public Module{
public:
    void OnInit() override;
    void OnExit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
private: 
    Ref<sol::state> lua;
};

void LuaScriptModuleInit();

}