#include "LuaScripts.h"
#include "OD/Core/ImGui.h"
#include "OD/Scene/SceneManager.h"
#include "OD/RenderPipeline/LightComponent.h"
#include <stdlib.h>

namespace OD{

void LuaScriptModuleInit(){
    SceneManager::Get().RegisterCoreComponent<LuaScriptComponent>("LuaScriptComponent");
    SceneManager::Get().RegisterSystem<LuaScriptSystem>("LuaScriptSystem");
}

void LuaScriptComponent::OnGui(Entity& e){
    LuaScriptComponent& script = e.GetComponent<LuaScriptComponent>();
    
    ImGui::DrawPath(std::string("scriptPath"), script.scriptPath, std::vector<std::string>{".lua"});
}

LuaScriptSystem::LuaScriptSystem(Scene* inScene):System(inScene){
    lua = CreateRef<sol::state>();
    lua->open_libraries(sol::lib::base, sol::lib::math, sol::lib::os, sol::lib::table, sol::lib::io, sol::lib::string);
    for(auto i: LuaBindsDB::Get().bindFuncs){
        i(*lua);
    } 
}

void LuaScriptSystem::Update(){
    auto scriptView = GetScene()->GetRegistry().view<LuaScriptComponent>();
    for(auto e: scriptView){
        LuaScriptComponent& luaScript = scriptView.get<LuaScriptComponent>(e);

        if(luaScript.scriptPath.empty() == false && luaScript.hasInited == false){
            auto result = lua->script_file(luaScript.scriptPath);
                                  
            sol::function OnStart = (*lua)["OnStart"];
            sol::function OnDestroy = (*lua)["OnDestroy"];
            sol::function OnUpdate = (*lua)["OnUpdate"];

            luaScript.OnStart = OnStart;
            luaScript.OnDestroy = OnDestroy;
            luaScript.OnUpdate = OnUpdate;
            luaScript.hasInited = true;
        }

        if(luaScript.hasInited == true){
            if(luaScript.hasStarted == false){
                luaScript.hasStarted = true;

                (*lua)["entity"] = Entity(e, GetScene());
                auto error2 = luaScript.OnStart();

                if(error2.valid() == false){
                    sol::error err2 = error2;
                    LogError("Running OnStart script: %s", err2.what());
                }
            }

            (*lua)["entity"] = Entity(e, GetScene());    
            auto error = luaScript.OnUpdate();

            if(error.valid() == false){
                sol::error err = error;
                LogError("Running OnUpdate script: %s", err.what());
            }
        }
    }   
}

}