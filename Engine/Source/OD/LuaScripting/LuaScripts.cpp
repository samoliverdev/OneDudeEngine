#include "LuaScripts.h"
#include "OD/Core/ImGui.h"
#include "OD/Scene/SceneManager.h"
#include "OD/RenderPipeline/LightComponent.h"
#include "OD/Core/Application.h"
#include <stdlib.h>

namespace OD{

void LuaScriptModuleInit(){
    SceneManager::Get().RegisterCoreComponent<LuaScriptComponent>("LuaScriptComponent");
    SceneManager::Get().RegisterSystem<LuaScriptSystem>("LuaScriptSystem");
    Application::AddModule(new LuaModule());
}

void LuaScriptComponent::OnGui(Entity& e, Scene& scene){
    LuaScriptComponent& script = scene.GetComponent<LuaScriptComponent>(e);
    
    std::string label = "scriptPath";
    std::vector<std::string> extension = std::vector<std::string>{".lua"};
    ImGui::DrawPath(label, script.scriptPath, extension);
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
            //sol::table result = lua->script_file(luaScript.scriptPath);
            auto result = lua->script_file(luaScript.scriptPath);

            //sol::table f = lua->load_file("").call();
                                  
            sol::function OnStart = (*lua)["OnStart"]; // result["OnStart"];
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

                (*lua)["entity"] = e; //Entity(e, GetScene());
                (*lua)["scene"] = scene;
                auto error2 = luaScript.OnStart();

                if(error2.valid() == false){
                    sol::error err2 = error2;
                    LogError("Running OnStart script: %s", err2.what());
                }
            }

            (*lua)["entity"] = e; //Entity(e, GetScene());   
            (*lua)["scene"] = scene; 
            auto error = luaScript.OnUpdate();

            if(error.valid() == false){
                sol::error err = error;
                LogError("Running OnUpdate script: %s", err.what());
            }
        }
    }   
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