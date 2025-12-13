#include "OD/pch.h"
#include "LuaScripts.h"
#include "OD/Core/ImGui.h"
#include "OD/Scene/SceneManager.h"
#include "OD/RenderPipeline/LightComponent.h"
#include "OD/Core/Application.h"
#include "OD/Core/Instrumentor.h"
#include <stdlib.h>

namespace OD{

void LuaScriptModuleInit(){
    SceneManager::Get().RegisterCoreComponent<LuaScriptComponent>("LuaScriptComponent", "Script");
    SceneManager::Get().RegisterSystem<LuaScriptSystem>("LuaScriptSystem");
    Application::AddModule(new LuaModule());
}

void renderLuaObjectProperties(sol::table& luaObject) {
    if(!luaObject.valid()) return;

    for(auto& pair : luaObject){
        sol::object key = pair.first;
        sol::object value = pair.second;

        // Skip non-string keys (we only care about named properties)
        if (!key.is<std::string>()) continue;

        std::string keyName = key.as<std::string>();

        if (value.is<double>()) {
            double val = value.as<double>();
            if (ImGui::InputDouble(keyName.c_str(), &val)) {
                luaObject.set(key, sol::make_object(luaObject.lua_state(), val));
            }
        }
        if (value.is<bool>()) {
            bool val = value.as<bool>();
            if (ImGui::Checkbox(keyName.c_str(), &val)) {
                luaObject.set(key, sol::make_object(luaObject.lua_state(), val));
            }
        }
        if (value.is<std::string>()) {
            std::string str = value.as<std::string>();
            char buffer[256];
            std::strncpy(buffer, str.c_str(), sizeof(buffer));
            buffer[sizeof(buffer) - 1] = '\0';

            if (ImGui::InputText(keyName.c_str(), buffer, sizeof(buffer))) {
                luaObject.set(key, sol::make_object(luaObject.lua_state(), std::string(buffer)));
            }
        }
        if(value.is<Vector3*>()){
            Vector3* vec = value.as<Vector3*>();
        
            float v[3] = { vec->x, vec->y, vec->z };
            if (ImGui::InputFloat3(keyName.c_str(), v)) {
                vec->x = v[0];
                vec->y = v[1];
                vec->z = v[2];
            }
        }
    }
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

sol::table toSolTable(sol::state& lua, const LuaTable& luaTable);

sol::object toSolObject(sol::state& lua, const LuaValue& val) {
    return std::visit(overloaded{
        [&](int v)         { return sol::make_object(lua, v); },
        [&](float v)       { return sol::make_object(lua, v); },
        [&](bool v)        { return sol::make_object(lua, v); },
        [&](const std::string& v) { return sol::make_object(lua, v); },
        [&](const LuaTable& tbl) { return sol::make_object(lua, toSolTable(lua, tbl)); },
        [&](const Vector3& vec){ return sol::make_object(lua, vec); },
    }, val);
}

sol::table toSolTable(sol::state& lua, const LuaTable& luaTable) {
    sol::table table = lua.create_table();
    for (const auto& [key, val] : luaTable) {
        table[key] = toSolObject(lua, val);
    }
    return table;
}

void applySolTable(sol::state& lua, sol::table& table, const LuaTable& luaTable) {
    for (const auto& [key, val] : luaTable) {
        table[key] = toSolObject(lua, val);
    }
}

LuaValue convertSolObject(const sol::object& obj) {
    switch (obj.get_type()) {
        case sol::type::number: {
            double val = obj.as<double>();
            if (val == static_cast<int>(val))
                return static_cast<int>(val);
            return static_cast<float>(val);
        }
        case sol::type::boolean:
            return obj.as<bool>();
        case sol::type::string:
            return obj.as<std::string>();
        case sol::type::userdata: {
            if(obj.is<Vector3>()){ return obj.as<Vector3>(); }
            break; // Ignore other userdata types
        }
        case sol::type::table: {
            LuaTable table;
            sol::table tbl = obj;
            for (auto& pair : tbl) {
                if (pair.first.get_type() == sol::type::string &&
                    pair.second.get_type() != sol::type::function &&
                    //pair.second.get_type() != sol::type::userdata &&
                    //pair.second.get_type() != sol::type::lightuserdata &&
                    pair.second.get_type() != sol::type::thread
                ){
                    std::string key = pair.first.as<std::string>();
                    table[key] = convertSolObject(pair.second);
                }
            }
            return table;
        }
        default:
            return {}; // you can define a NilValue type if needed
    }

    return {};
}

LuaScriptComponent::LuaScriptComponent(const LuaScriptComponent& other){
    scriptPath = other.scriptPath;
    if(other.data.valid() == true){
        saveData = convertSolObject(other.data);
    } else {
        saveData = other.saveData;
    }
}

LuaScriptComponent::LuaScriptComponent(LuaScriptComponent&& other){
    scriptPath = std::move(other.scriptPath);
    if(other.data.valid() == true){
        saveData = convertSolObject(other.data);
    } else {
        saveData = std::move(other.saveData);
    }
}

LuaScriptComponent& LuaScriptComponent::operator=(const LuaScriptComponent& other){
    if(this == &other) return *this;
    scriptPath = other.scriptPath;
    if(other.data.valid() == true){
        saveData = convertSolObject(other.data);
    } else {
        saveData = other.saveData;
    }
    return *this;
}

LuaScriptComponent& LuaScriptComponent::operator=(LuaScriptComponent&& other){
    if(this == &other) return *this;
    scriptPath = std::move(other.scriptPath);
    if(other.data.valid() == true){
        saveData = convertSolObject(other.data);
    } else {
        saveData = std::move(other.saveData);
    }
    return *this;
}

void LuaScriptComponent::OnGui(Entity& e, Scene& scene){
    LuaScriptComponent& script = scene.GetComponent<LuaScriptComponent>(e);
    
    std::string label = "scriptPath";
    std::vector<std::string> extension = std::vector<std::string>{".lua"};
    ImGui::DrawPath(label, script.scriptPath, extension);

    //if(script.data.valid() == true && script.data.empty() == false) 
    renderLuaObjectProperties(script.data);
}

void LuaScriptSystem::OnInit(Scene& scene){
    lua = CreateRef<sol::state>();
    lua->open_libraries(sol::lib::base, sol::lib::math, sol::lib::os, sol::lib::table, sol::lib::io, sol::lib::string);
    for(auto i: LuaBindsDB::Get().bindFuncs){
        i(*lua);
    } 

    scene.GetRegistry().on_destroy<LuaScriptComponent>().connect<&OnDestroyScript>();
}


void LuaScriptSystem::OnEnd(Scene& scene){
    scene.GetRegistry().on_destroy<LuaScriptComponent>().disconnect<&OnDestroyScript>();
}

void LuaScriptSystem::Update(Scene& scene){
    OD_PROFILE_SCOPE("LuaScriptSystem::OnUpdate");

    auto scriptView = scene.GetRegistry().view<LuaScriptComponent>();
    for(auto e: scriptView){
        LuaScriptComponent& luaScript = scriptView.get<LuaScriptComponent>(e);

        if(luaScript.scriptPath.empty() == false && luaScript.hasInited == false){
            //sol::table result = lua->script_file(luaScript.scriptPath);
            /*auto result = lua->script_file(luaScript.scriptPath);
            //luaScript.data = (*lua)["Data"];
            //sol::table f = lua->load_file("").call();
                                  
            sol::function OnStart = (*lua)["OnStart"]; // result["OnStart"];
            sol::function OnDestroy = (*lua)["OnDestroy"];
            sol::function OnUpdate = (*lua)["OnUpdate"];

            luaScript.OnStart = OnStart;
            luaScript.OnDestroy = OnDestroy;
            luaScript.OnUpdate = OnUpdate;
            luaScript.hasInited = true;*/

            luaScript.data = lua->script_file(luaScript.scriptPath); 
            applySolTable(*lua, luaScript.data, std::get<LuaTable>(luaScript.saveData));
            luaScript.OnStart = luaScript.data["OnStart"];
            luaScript.OnDestroy = luaScript.data["OnDestroy"];
            luaScript.OnUpdate = luaScript.data["OnUpdate"];
            luaScript.hasInited = true;
            //luaScript.saveData = convertSolObject(luaScript.data);
        }

        if(scene.Running() == false) continue;

        if(luaScript.hasInited == true){
            if(luaScript.hasStarted == false){
                luaScript.hasStarted = true;

                (*lua)["entity"] = e; //Entity(e, GetScene());
                (*lua)["scene"] = &scene;
                //luaScript.data["entity"] = e;
                //luaScript.data["scene"] = scene;
                auto error2 = luaScript.OnStart(luaScript.data);

                if(error2.valid() == false){
                    sol::error err2 = error2;
                    LogError("Running OnStart script: %s", err2.what());
                }
            }

            (*lua)["entity"] = e; //Entity(e, GetScene());   
            (*lua)["scene"] = &scene; 
            //luaScript.data["entity"] = e;
            //luaScript.data["scene"] = scene;
            auto error = luaScript.OnUpdate(luaScript.data);

            if(error.valid() == false){
                sol::error err = error;
                LogError("Running OnUpdate script: %s", err.what());
            }
        }
    }   
}

void LuaScriptSystem::OnDestroyScript(entt::registry & r, entt::entity e){

}

void LuaModule::OnInit(){
    lua = CreateRef<sol::state>();
    lua->open_libraries(sol::lib::base, sol::lib::math, sol::lib::os, sol::lib::table, sol::lib::io, sol::lib::string);
    for(auto i: LuaBindsDB::Get().bindFuncs){
        i(*lua);
    } 

    auto FileExists = [](const std::string& name){
        std::ifstream f(name);
        return f.good();
    };

    if(FileExists("Main.lua")){
        lua->script_file("Main.lua");
        return;
    }
    if(FileExists("main.lua")){
        lua->script_file("main.lua");
        return;
    }
}

void LuaModule::OnExit(){}
void LuaModule::OnUpdate(float deltaTime){}
void LuaModule::OnRender(float deltaTime){}
void LuaModule::OnGUI(){}
void LuaModule::OnResize(int width, int height){}

}