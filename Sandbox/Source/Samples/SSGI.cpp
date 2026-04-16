#include "SSGI.h"

#include "OD/pch.h"
#include "LoadScene.h"
#include "Standard/Module.h"
#include <OD/Core/Application.h>
#include <OD/Scene/SceneManager.h>
#include <OD/Editor/Editor.h>

void SSGISample::OnInit(){
    LogInfo("Game Init");
    Application::Vsync(false);

    Standard::ModuleInit();

    auto& SceneManager = SceneManager::Get();
    OD::Ref<OD::Scene> scene = SceneManager.NewScene();

    scene->Load("Sandbox/Scenes/SSGI.scene");

    Application::AddModule<Editor>();
    //scene->Start();
    //LogInfo("Tes");
}

void SSGISample::OnUpdate(float deltaTime){}   
void SSGISample::OnRender(float deltaTime){}
void SSGISample::OnGUI(){}
void SSGISample::OnResize(int width, int height){}
void SSGISample::OnExit(){}