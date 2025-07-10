#include "LoadScene.h"
#include "StandardAsset/Module.h"

void LoadSceneSample::OnInit(){
    LogInfo("Game Init");
    Application::Vsync(false);

    Standard::ModuleInit();

    auto& SceneManager = SceneManager::Get();
    OD::Scene* scene = SceneManager.NewScene();

    scene->Load("StandardAsset/Scenes/Prototype.scene");

    Application::AddModule<Editor>();
    //scene->Start();
}

void LoadSceneSample::OnUpdate(float deltaTime){}   
void LoadSceneSample::OnRender(float deltaTime){}
void LoadSceneSample::OnGUI(){}
void LoadSceneSample::OnResize(int width, int height){}
void LoadSceneSample::OnExit(){}