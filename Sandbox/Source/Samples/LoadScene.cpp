#include "LoadScene.h"
#include "Standard/Module.h"
#include <OD/Core/Application.h>
#include <OD/Scene/SceneManager.h>
#include <OD/Editor/Editor.h>

void LoadSceneSample::OnInit(){
    LogInfo("Game Init");
    Application::Vsync(true);

    Standard::ModuleInit();

    auto& SceneManager = SceneManager::Get();
    OD::Scene* scene = SceneManager.NewScene();

    scene->Load("Standard/Scenes/Prototype.scene");
    //scene->Load("Sandbox/Scenes/TerrainTest2.scene");

    Application::AddModule<Editor>();
    //scene->Start();
}

void LoadSceneSample::OnUpdate(float deltaTime){}   
void LoadSceneSample::OnRender(float deltaTime){}
void LoadSceneSample::OnGUI(){}
void LoadSceneSample::OnResize(int width, int height){}
void LoadSceneSample::OnExit(){}