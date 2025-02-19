#include "CoreModulesStartup.h"
#include "OD/Core/Application.h"
#include "OD/Core/Input.h"
#include "OD/Core/Asset.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Scene/Scene.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Scene/Scripts.h"
#include "OD/Animation/Animator.h"
#include "OD/Physics/PhysicsSystem.h"  
#include "OD/RenderPipeline/StandRenderPipeline.h"
#include "OD/Audio/AudioClip.h"
#include "OD/Audio/AudioSystem.h"
#include "OD/Navmesh/Navmesh.h"
#include "OD/LuaScripting/LuaScripts.h"
#include "OD/Terrain/Terrain.h"
#include <filesystem>

namespace OD{

void RegisterMathLuaBindings(sol::state& lua){
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

void CoreModuleInit(){
    LuaBindsDB::Get().RegisterLuaBind([](sol::state& lua){
        lua["LogInfo"] = [](const std::string& text){ LogInfo(text.c_str()); };
        lua["LogWarning"] = [](const std::string& text){ LogWarning(text.c_str()); };
        lua["LogError"] = [](const std::string& text){ LogError(text.c_str()); };
    });
    LuaBindsDB::Get().RegisterLuaBind(RegisterMathLuaBindings);
    LuaBindsDB::Get().RegisterLuaBind<Color>();
    LuaBindsDB::Get().RegisterLuaBind<Application>();
    LuaBindsDB::Get().RegisterLuaBind<Input>();
}

void CoreModulesStartup(){
    LogInfo("CoreModulesStartup");
    CoreModuleInit();
    GraphicsModuleInit();
    StandRenderPipelineModuleInit();
    #if !defined(__EMSCRIPTEN__)
    PhysicsModuleInit();
    #endif
    ScriptModuleInit();
    AnimatorModuleInit();
    AudioModuleInit();
    NavmeshModuleInit();
    SceneManagerModuleInit();
    TerrainModuleInit();
    LuaScriptModuleInit();
}

}