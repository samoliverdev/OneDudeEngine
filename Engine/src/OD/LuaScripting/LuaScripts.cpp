#include "LuaScripts.h"
#include "OD/Scene/SceneManager.h"
#include "OD/RenderPipeline/LightComponent.h"

namespace OD{

void LuaScriptModuleInit(){
    SceneManager::Get().RegisterCoreComponent<LuaScriptComponent>("LuaScriptComponent");
    SceneManager::Get().RegisterSystem<LuaScriptSystem>("LuaScriptSystem");
}

void LuaScriptComponent::OnGui(Entity& e){
    
}

LuaScriptSystem::LuaScriptSystem(Scene* inScene):System(inScene){
    lua = CreateRef<sol::state>();
    lua->open_libraries(sol::lib::base, sol::lib::math, sol::lib::os, sol::lib::table, sol::lib::io, sol::lib::string);
    RegisterLuaBindings(*lua, GetScene());
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

void LuaScriptSystem::RegisterLuaBindings(sol::state& lua, Scene* scene){
    lua["LogInfo"] = [](std::string& text){
        LogInfo(text.c_str());
    };
    RegisterMathLuaBindings(lua);
    
    Entity::CreateLuaBind(lua);
    TransformComponent::CreateLuaBind(lua);
    InfoComponent::CreateLuaBind(lua);
    LightComponent::CreateLuaBind(lua);

    Entity::RegisterMetaComponent<TransformComponent>();
    Entity::RegisterMetaComponent<InfoComponent>();
    Entity::RegisterMetaComponent<LightComponent>();
}

void LuaScriptSystem::RegisterMathLuaBindings(sol::state& lua){
    //--------------Vector2--------------
    auto vec2MultiplyOverloads = sol::overload(
        [](const glm::vec2& v1, const glm::vec2& v2){ return v1 * v2; },
        [](const glm::vec2& v1, float value){ return v1 * value; },
        [](float value, const glm::vec2& v1){ return v1 * value; }
    );
    auto vec2DivideOverloads = sol::overload(
        [](const glm::vec2& v1, const glm::vec2& v2){ return v1 / v2; },
        [](const glm::vec2& v1, float value){ return v1 / value; },
        [](float value, const glm::vec2& v1){ return v1 / value; }
    );
    auto vec2AdditionOverloads = sol::overload(
        [](const glm::vec2& v1, const glm::vec2& v2){ return v1 + v2; },
        [](const glm::vec2& v1, float value){ return v1 + value; },
        [](float value, const glm::vec2& v1){ return v1 + value; }
    );
    auto vec2SubtractionOverloads = sol::overload(
        [](const glm::vec2& v1, const glm::vec2& v2){ return v1 - v2; },
        [](const glm::vec2& v1, float value){ return v1 - value; },
        [](float value, const glm::vec2& v1){ return v1 - value; }
    );
    lua.new_usertype<glm::vec2>(
        "Vector2",
        sol::call_constructor,
        sol::constructors<glm::vec2(), glm::vec2(float), glm::vec2(float, float)>(),
        "x", &glm::vec2::x,
        "y", &glm::vec2::y,
        sol::meta_function::multiplication, vec2MultiplyOverloads,
        sol::meta_function::division, vec2DivideOverloads,
        sol::meta_function::addition, vec2AdditionOverloads,
        sol::meta_function::subtraction, vec2SubtractionOverloads
    );

    //--------------Vector3--------------
    auto vec3MultiplyOverloads = sol::overload(
        [](const glm::vec3& v1, const glm::vec3& v2){ return v1 * v2; },
        [](const glm::vec3& v1, float value){ return v1 * value; },
        [](float value, const glm::vec3& v1){ return v1 * value; }
    );
    auto vec3DivideOverloads = sol::overload(
        [](const glm::vec3& v1, const glm::vec3& v2){ return v1 / v2; },
        [](const glm::vec3& v1, float value){ return v1 / value; },
        [](float value, const glm::vec3& v1){ return v1 / value; }
    );
    auto vec3AdditionOverloads = sol::overload(
        [](const glm::vec3& v1, const glm::vec3& v2){ return v1 + v2; },
        [](const glm::vec3& v1, float value){ return v1 + value; },
        [](float value, const glm::vec3& v1){ return v1 + value; }
    );
    auto vec3SubtractionOverloads = sol::overload(
        [](const glm::vec3& v1, const glm::vec3& v2){ return v1 - v2; },
        [](const glm::vec3& v1, float value){ return v1 - value; },
        [](float value, const glm::vec3& v1){ return v1 - value; }
    );
    lua.new_usertype<glm::vec3>(
        "Vector3",
        sol::call_constructor,
        sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>(),
        "x", &glm::vec3::x,
        "y", &glm::vec3::y,
        "z", &glm::vec3::z,
        sol::meta_function::multiplication, vec3MultiplyOverloads,
        sol::meta_function::division, vec3DivideOverloads,
        sol::meta_function::addition, vec3AdditionOverloads,
        sol::meta_function::subtraction, vec3SubtractionOverloads
    );

    //--------------Vector4--------------
    auto vec4MultiplyOverloads = sol::overload(
        [](const glm::vec4& v1, const glm::vec4& v2){ return v1 * v2; },
        [](const glm::vec4& v1, float value){ return v1 * value; },
        [](float value, const glm::vec4& v1){ return v1 * value; }
    );
    auto vec4DivideOverloads = sol::overload(
        [](const glm::vec4& v1, const glm::vec4& v2){ return v1 / v2; },
        [](const glm::vec4& v1, float value){ return v1 / value; },
        [](float value, const glm::vec4& v1){ return v1 / value; }
    );
    auto vec4AdditionOverloads = sol::overload(
        [](const glm::vec4& v1, const glm::vec4& v2){ return v1 + v2; },
        [](const glm::vec4& v1, float value){ return v1 + value; },
        [](float value, const glm::vec4& v1){ return v1 + value; }
    );
    auto vec4SubtractionOverloads = sol::overload(
        [](const glm::vec4& v1, const glm::vec4& v2){ return v1 - v2; },
        [](const glm::vec4& v1, float value){ return v1 - value; },
        [](float value, const glm::vec4& v1){ return v1 - value; }
    );
    lua.new_usertype<glm::vec4>(
        "Vector4",
        sol::call_constructor,
        sol::constructors<glm::vec4(), glm::vec4(float), glm::vec4(float, float, float, float)>(),
        "x", &glm::vec4::x,
        "y", &glm::vec4::y,
        "z", &glm::vec4::z,
        "w", &glm::vec4::w,
        sol::meta_function::multiplication, vec4MultiplyOverloads,
        sol::meta_function::division, vec4DivideOverloads,
        sol::meta_function::addition, vec4AdditionOverloads,
        sol::meta_function::subtraction, vec4SubtractionOverloads
    );

    //--------------Quaternion--------------
    auto qualtMultiplyOverloads = sol::overload(
        [](const glm::quat& v1, const glm::quat& v2){ return v1 * v2; },
        [](const glm::quat& v1, float value){ return v1 * value; },
        [](float value, const glm::quat& v1){ return v1 * value; }
    );
    lua.new_usertype<glm::quat>(
        "Quaternion",
        sol::call_constructor,
        sol::constructors<glm::quat(), glm::quat(float, float, float, float)>(),
        "x", &glm::quat::x,
        "y", &glm::quat::y,
        "z", &glm::quat::z,
        "w", &glm::quat::w,
        sol::meta_function::multiplication, vec4MultiplyOverloads
    );

    //--------------Math Funcs--------------
    lua.set_function("MathDistance", sol::overload(
        [](glm::vec2& a, glm::vec2& b){ return glm::distance(a, b); },
        [](glm::vec3& a, glm::vec3& b){ return glm::distance(a, b); },
        [](glm::vec4& a, glm::vec4& b){ return glm::distance(a, b); }
    ));
}

}