#include "Application.h"
#include "OD/Defines.h"
#include "OD/Platform/Platform.h"
#include "OD/Graphics/Graphics.h"
#include "ImGui.h"
#include "Input.h"
#include "Instrumentor.h"
#include "JobSystem.h"
#include "OD/CoreModulesStartup.h"
#include <algorithm>

namespace OD{

std::vector<std::string> args;
std::vector<Module*> modules;

Module* mainModule;
bool running = true;
int width;
int heigth;

float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; 

ProjectSettings settings;

bool Application::Create(Module* inMainModule, ApplicationConfig appConfig, const char* projectPath){
    width = appConfig.startWidth;
    heigth = appConfig.startHeight;

    std::string projectSettingsPath = std::string(projectPath) + "/ProjectSettings.json";
    if(projectPath[0] == '\0') projectSettingsPath = "ProjectSettings.json";

    //cereal::JSONInputArchive ar2(std::strstream(NULL, 0));

    std::ifstream stream(projectSettingsPath);
    if(stream.fail()){
        std::ofstream os(projectSettingsPath);
        cereal::JSONOutputArchive ar(os);
        ArchiveDumpNamed(ar, "ProjectSettings", settings);
    } else {
        cereal::JSONInputArchive ar{stream};
        ArchiveDumpNamed(ar, "ProjectSettings", settings);
    }
    std::filesystem::current_path(projectPath);
    LogInfo("Project Path: %s", projectPath);

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
        Instrumentor::BeginLoop();

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

        Instrumentor::EndLoop();
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

ProjectSettings& Application::GetProjectSettings(){
    return settings;
}

void Application::RemoveModule(Module* module){
    if(std::find(modules.begin(), modules.end(), module) == modules.end()){
        LogError("Trying remove module with has not added");
        return;
    }

    modules.erase(std::remove(modules.begin(), modules.end(), module), modules.end());
    module->OnExit();
}

void Application::AddModule(Module* module){
    modules.push_back(module);
    modules.back()->OnInit();
}

const std::vector<Module*>& Application::GetAllModules(){
    return modules;
}

std::vector<std::string>& Application::GetArgs(){
    return args;
}

void Application::OnExit(){
    //LogInfo("Application::OnExit");

    for(auto i: modules){
        i->OnExit();
        //delete i;
    }

    Graphics::Shutdown();
    //Input::_Shutdown(0);
    Platform::SystemShutdown(0);
}

}