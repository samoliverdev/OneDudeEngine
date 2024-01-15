#include "Application.h"
#include "OD/Platform/Platform.h"
#include "OD/Graphics/Graphics.h"
#include "ImGui.h"
#include "Input.h"
#include "Instrumentor.h"
#include "JobSystem.h"
#include "AssetManager.h"
#include "OD/CoreModulesStartup.h"

namespace OD{

std::vector<std::string> Application::args;
std::vector<Module*> Application::modules;

Module* mainModule;
bool running = true;
int width;
int heigth;

float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; 

bool Application::Create(Module* inMainModule, ApplicationConfig appConfig){
    width = appConfig.startWidth;
    heigth = appConfig.startHeight;

    if(Platform::SystemStartup(
        appConfig.name.c_str(), 
        appConfig.startPosX, 
        appConfig.startPosY,
        appConfig.startWidth,
        appConfig.startHeight) == false) return false;

    Graphics::Initialize();
    //Input::_Initialize(0, 0);
    JobSystem::Initialize();
    //AssetTypesDB::_Init();
    CoreModulesStartup();

    mainModule = inMainModule;
    AddModule(mainModule);

    running = true;
    return true;
}

bool Application::Run(){
    while(running){
        float currentFrame = Platform::GetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame; 

        //Input::Update();
        Platform::PumpMessages();
        Platform::PreUpdate();
        Input::Update();

        {
            OD_PROFILE_SCOPE("Application::Run::OnUpdate");
            for(auto i: modules) i->OnUpdate(deltaTime);
        }
        {
            OD_PROFILE_SCOPE("Application::Run::OnRender");
            for(auto i: modules) i->OnRender(deltaTime);
        }
        {
            OD_PROFILE_SCOPE("Application::Run::OnGUI");
            for(auto i: modules) i->OnGUI();
        }

        Platform::LateUpdate();
        Platform::SwapBuffers();
    }

    OnExit();

    return true;
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

void Application::OnExit(){
    LogInfo("Application::OnExit");

    for(auto i: modules){
        i->OnExit();
        //delete i;
    }

    Graphics::Shutdown();
    //Input::_Shutdown(0);
    Platform::SystemShutdown(0);
}

}