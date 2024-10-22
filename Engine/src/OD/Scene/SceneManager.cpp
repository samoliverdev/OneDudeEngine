#include "SceneManager.h"
#include "OD/Core/Application.h"

namespace OD{

void SceneManagerModuleInit(){
    Application::AddModule(&SceneManager::Get());
}

SceneManager& SceneManager::Get(){
    static SceneManager sceneManager;
    return sceneManager;
}

void SceneManager::OnInit(){}

void SceneManager::OnExit(){
    if(activeScene != nullptr) delete activeScene;
}

void SceneManager::OnUpdate(float deltaTime){
    if(GetActiveScene() == nullptr) return;
    GetActiveScene()->Update();
}

void SceneManager::OnRender(float deltaTime){
    if(GetActiveScene() == nullptr) return;
    GetActiveScene()->Draw();
}

void SceneManager::OnGUI(){}
void SceneManager::OnResize(int width, int height){}

SceneManager::SceneState SceneManager::GetSceneState(){ 
    return sceneState; 
}

bool SceneManager::InEditor(){ 
    return inEditor; 
}

Scene* SceneManager::GetActiveScene(){ 
    //if(activeScene == nullptr) return NewScene();
    return activeScene; 
}

void SceneManager::SetActiveScene(Scene* s){ 
    activeScene = s; 
}

Scene* SceneManager::NewScene(){
    delete activeScene;
    activeScene = new Scene();
    return activeScene;
}

}