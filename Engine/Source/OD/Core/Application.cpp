#include "Application.h"
#include "OD/Defines.h"
#include "OD/Platform/Platform.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/GraphicsDevice.h"
#include "OD/CoreModulesStartup.h"
#include "OD/Serialization/SerializationFull.h"
#include "Project.h"
#include "ImGui.h"
#include "Input.h"
#include "Instrumentor.h"
#include "JobSystem.h"
#include "Lua.h"
#include <algorithm>
#include <fstream>
#include <string>

namespace OD{

std::vector<std::string> args;
std::vector<Module*> modules;
std::vector<Module*> modulesToAdd;
std::vector<Module*> modulesToRemove;
bool inUpdate = false;

Action<void()> onFrameEnd;

Module* mainModule;
bool running = true;
int width;
int heigth;

float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; 

extern GraphicsDevice* graphicsDevice;

bool Application::Create(Module* inMainModule, ApplicationConfig appConfig, const char* projectPath){
    auto project = ProjectManager::LoadProject(projectPath);
    if(project == nullptr) return false;

    auto exists = [](const char *fname){
        FILE *file;
        if((file = fopen(fname, "r"))){
            fclose(file);
            return 1;
        }
        return 0;
    };

    auto FileExists = [](const std::string& name){
        std::ifstream f(name);
        return f.good();
    };


    LogInfo("File exist: %s %d", "Engine/Fonts/fa-solid-900.ttf", exists("Engine/Fonts/fa-solid-900.ttf"));

    width = appConfig.startWidth;
    heigth = appConfig.startHeight;

    Graphics::SelectGraphicsDevice();

    if(Platform::SystemStartup(
        appConfig.name.c_str(), 
        appConfig.startPosX, 
        appConfig.startPosY,
        appConfig.startWidth,
        appConfig.startHeight) == false) return false;

    Graphics::Initialize();
    //Input::_Initialize(0, 0);
    #ifdef __EMSCRIPTEN__
    #else
    JobSystem::Initialize();
    #endif
    //AssetTypesDB::_Init();
    //CoreModulesStartup();

    LogInfo("Test2");

    mainModule = inMainModule;
    AddModule(mainModule);

    running = true;
    return true;
}

//#include "OD/Platform/OpenGL/GL.h"
//#include <GLFW/glfw3.h>

void Application::Loop(){
    #if OD_PROFILE
    Instrumentor::BeginLoop();
    #endif

    {
    OD_PROFILE_SCOPE("Application::Run");

    float currentFrame = Platform::GetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame; 

    Platform::PumpMessages();
    //Platform::PreUpdate();
    //Graphics::_Begin();
    Input::Update();

    for(auto i: modulesToAdd) _AddModule(i);
    modulesToAdd.clear();

    inUpdate = true;
    {
        OD_PROFILE_SCOPE("Application::Run::OnUpdate");
        for(auto i: modules) i->OnUpdate(deltaTime);
    }

    //Platform::SwapBuffers();
    Platform::PreUpdate();
    Graphics::_Begin();
    {
        OD_PROFILE_SCOPE("Application::Run::OnRender");
        for(auto i: modules) i->OnRender(deltaTime);
    }
    {
        if(graphicsDevice->ImGuiSupport()){
            OD_PROFILE_SCOPE("Application::Run::OnGUI");
            for(auto i: modules) i->OnGUI();
        }
        //#endif
    }
    inUpdate = false;

    {
    OD_PROFILE_SCOPE("Application::Run::3");
    for(auto i: modulesToRemove) _RemoveModule(i);
    modulesToRemove.clear();
    onFrameEnd.Invoke();
    }

    Graphics::_End();
    Platform::LateUpdate();
    Platform::SwapBuffers();
    }

    #if OD_PROFILE
    Instrumentor::EndLoop();
    #endif
}

bool Application::Run(){
    /*while(running){
        #if OD_PROFILE
        Instrumentor::BeginLoop();
        #endif

        {
        OD_PROFILE_SCOPE("Application::Run");

        float currentFrame = Platform::GetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame; 

        Platform::PumpMessages();
        //Platform::PreUpdate();
        //Graphics::_Begin();
        Input::Update();

        for(auto i: modulesToAdd) _AddModule(i);
        modulesToAdd.clear();

        inUpdate = true;
        {
            OD_PROFILE_SCOPE("Application::Run::OnUpdate");
            for(auto i: modules) i->OnUpdate(deltaTime);
        }
        //Platform::SwapBuffers();
        Platform::PreUpdate();
        Graphics::_Begin();
        {
            OD_PROFILE_SCOPE("Application::Run::OnRender");
            for(auto i: modules) i->OnRender(deltaTime);
        }
        {
            OD_PROFILE_SCOPE("Application::Run::OnGUI");
            for(auto i: modules) i->OnGUI();
        }
        inUpdate = false;

        {
        OD_PROFILE_SCOPE("Application::Run::3");
        for(auto i: modulesToRemove) _RemoveModule(i);
        modulesToRemove.clear();
        onFrameEnd.Invoke();
        }

        Graphics::_End();
        Platform::LateUpdate();
        Platform::SwapBuffers();
        }

        #if OD_PROFILE
        Instrumentor::EndLoop();
        #endif
    }*/

#ifdef __EMSCRIPTEN__
    LogInfo("EMSCRIPTEN Loop");
    emscripten_set_main_loop(Loop, 0, true);
#else
    while(running) Loop();
#endif

    OnExit();

    return true;
}

void Application::OnExit(){
    //LogInfo("Application::OnExit");

    for(auto i: modules){
        i->OnExit();
        if(i->DeleteOnExit()) delete i;
    }
    modules.clear();

    onFrameEnd.Clean();

    AssetManager::Get().UnloadAll();
    AssetTypesDB::Get().assetFuncs.clear();

    Graphics::Shutdown();
    //Input::_Shutdown(0);
    Platform::SystemShutdown(0);
}

void Application::Quit(){
    running = false;
}

void Application::Exit(){
    OnExit();
    exit(0);
}

void Application::GetFramebufferSize(int* width, int* height){}

float Application::DeltaTime(){ return deltaTime; }
int Application::ScreenWidth(){ return width; }
int Application::ScreenHeight(){ return heigth; }

void Application::Vsync(bool enabled){ Platform::SetVSync(enabled); }
bool Application::Vsync(){ return Platform::IsVSync(); }

void Application::_OnResize(int inWidth, int inHeight){
    width = inWidth;
    heigth = inHeight;

    //mainModule->OnResize(_width, _height);
    for(auto i: modules) i->OnResize(width, heigth);
}

Action<void()>& Application::GetOnFrameEnd(){
    return onFrameEnd;
}

void Application::RemoveModule(Module* module){
    if(inUpdate == false){
        _RemoveModule(module);
        return;
    }

    if(std::find(modules.begin(), modules.end(), module) == modules.end()){
        LogError("Trying remove module with has not added");
        return;
    }

    modulesToRemove.push_back(module);
}

void Application::AddModule(Module* module){
    if(inUpdate == false){
        _AddModule(module);
        return;
    }

    modulesToAdd.push_back(module);
}

void Application::_RemoveModule(Module* module){
    if(std::find(modules.begin(), modules.end(), module) == modules.end()){
        LogError("Trying remove module with has not added");
        return;
    }

    modules.erase(std::remove(modules.begin(), modules.end(), module), modules.end());
    module->OnExit();
}

void Application::_AddModule(Module* module){
    modules.push_back(module);
    modules.back()->OnInit();
}

const std::vector<Module*>& Application::GetAllModules(){
    return modules;
}

std::vector<std::string>& Application::GetArgs(){
    return args;
}

void Application::CreateLuaBind(sol::state& lua){
    lua.new_usertype<Application>(
        "Application",
        "ScreenWidth", Application::ScreenWidth,
        "ScreenHeight", Application::ScreenHeight,
        "DeltaTime", Application::DeltaTime,
        "Quit", Application::Quit,
        "Exit", Application::Exit,
        "Vsync", sol::overload(
            [](){ return Application::Vsync(); },
            [](bool v){ Application::Vsync(v); }
        )
    );
}

}