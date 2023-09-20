#include "Application.h"
#include "OD/Platform/Platform.h"
#include "OD/Renderer/Renderer.h"
#include "ImGui.h"
#include "Input.h"
#include "Instrumentor.h"

namespace OD{

std::vector<std::string> Application::args;
std::vector<Module*> Application::_modules;

Module* _mainModule;
bool _running = true;
int _width;
int _heigth;

float _deltaTime = 0.0f;	// Time between current frame and last frame
float _lastFrame = 0.0f; 

bool Application::Create(Module* mainModule, ApplicationConfig appConfig){
    _width = appConfig.startWidth;
    _heigth = appConfig.startHeight;

    if(Platform::SystemStartup(
        appConfig.name.c_str(), 
        appConfig.startPosX, 
        appConfig.startPosY,
        appConfig.startWidth,
        appConfig.startHeight) == false) return false;

    Renderer::_Initialize();
    //Input::_Initialize(0, 0);

    _mainModule = mainModule;
    AddModule(_mainModule);

    _running = true;
    return true;
}

bool Application::Run(){
    while (_running){
        float currentFrame = Platform::GetTime();
        _deltaTime = currentFrame - _lastFrame;
        _lastFrame = currentFrame; 

        //Input::_Update(0);
        Platform::PumpMessages();
        Platform::PreUpdate();

        {
            OD_PROFILE_SCOPE("Application::Run::OnUpdate");
            for(auto i: _modules) i->OnUpdate(_deltaTime);
        }
        {
            OD_PROFILE_SCOPE("Application::Run::OnRender");
            for(auto i: _modules) i->OnRender(_deltaTime);
        }
        {
            OD_PROFILE_SCOPE("Application::Run::OnGUI");
            for(auto i: _modules) i->OnGUI();
        }

        ImGui::Begin("Profile");
        for(auto i: Instrumentor::Get().results()){ 
            float durration = (i.end - i.start) * 0.001f;
            ImGui::Text("%s: %.3f.ms", i.name, durration);
        }
        Instrumentor::Get().results().clear();
        ImGui::End();

        Platform::LateUpdate();
        Platform::SwapBuffers();
    }

    Renderer::_Shutdown();
    //Input::_Shutdown(0);
    Platform::SystemShutdown(0);

    return true;
}

void Application::Quit(){
    _running = false;
}

void Application::GetFramebufferSize(int* width, int* height){}

float Application::deltaTime(){ return _deltaTime; }
int Application::screenWidth(){ return _width; }
int Application::screenHeight(){ return _heigth; }

void Application::vsync(bool enabled){ Platform::SetVSync(enabled); }
bool Application::vsync(){ return Platform::IsVSync(); }

void Application::_OnResize(int width, int height){
    _width = width;
    _heigth = height;

    //mainModule->OnResize(_width, _height);
    for(auto i: _modules) i->OnResize(_width, _heigth);
}

}