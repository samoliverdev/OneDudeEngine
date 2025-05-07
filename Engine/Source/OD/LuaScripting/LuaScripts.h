#pragma once
#include "OD/Defines.h"
#include "OD/Core/Lua.h"
#include "OD/Scene/Scene.h"
#include "OD/Serialization/Serialization.h"

namespace OD{

using LuaTable = std::map<std::string, struct LuaValue>;

struct LuaValue : std::variant<
    int, float, bool, std::string, LuaTable, Vector3
> {
    using Base = std::variant<int, float, bool, std::string, LuaTable, Vector3>;
    using Base::Base;

    /*template <class Archive>
    void serialize(Archive& ar) {
        std::visit([&](auto& val) {
            ar(val);
        }, *this);
    }*/

    template <class Archive>
    void save(Archive& ar) const {
        ar(cereal::make_nvp("index", this->index()));
        std::visit([&](auto& val) {
            ar(cereal::make_nvp("value", val));
        }, *this);
    }

    template <class Archive>
    void load(Archive& ar) {
        std::size_t index;
        ar(cereal::make_nvp("index", index));

        switch (index) {
            case 0: { int v; ar(cereal::make_nvp("value", v)); *this = v; break; }
            case 1: { float v; ar(cereal::make_nvp("value", v)); *this = v; break; }
            case 2: { bool v; ar(cereal::make_nvp("value", v)); *this = v; break; }
            case 3: { std::string v; ar(cereal::make_nvp("value", v)); *this = v; break; }
            case 4: { LuaTable v; ar(cereal::make_nvp("value", v)); *this = v; break; }
            case 5: { Vector3 v; ar(cereal::make_nvp("value", v)); *this = v; break; } // ← Vector3 case
            default:
                throw std::runtime_error("Invalid LuaValue variant index");
        }
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