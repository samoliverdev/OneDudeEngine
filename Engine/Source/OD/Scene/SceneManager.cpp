#include "SceneManager.h"
#include "OD/Core/Application.h"
#include "OD/Core/Lua.h"
#include "OD/Core/Instrumentor.h"

namespace OD{

void SceneManagerModuleInit(){
    Application::AddModule(&SceneManager::Get());

    LuaBindsDB::Get().RegisterLuaBind<TransformComponent>();
    LuaBindsDB::Get().RegisterLuaBind<InfoComponent>();
    LuaBindsDB::Get().RegisterLuaBind<EntityHandle>();
    LuaBindsDB::Get().RegisterLuaBind<Scene>();
}

SceneManager& SceneManager::Get(){
    static SceneManager sceneManager;
    return sceneManager;
}

void SceneManager::OnInit(){}

void SceneManager::OnExit(){
    if(activeScene != nullptr){
        delete activeScene;
        activeScene = nullptr;
    }

    coreComponentsSerializer.clear();
    componentsSerializer.clear();
    scriptsSerializer.clear();
    addSystemFuncs.clear();
}

void SceneManager::OnUpdate(float deltaTime){
    OD_PROFILE_SCOPE("SceneManager::OnUpdate");

    if(GetActiveScene() == nullptr) return;
    GetActiveScene()->Update();
}

void SceneManager::OnRender(float deltaTime){
    OD_PROFILE_SCOPE("SceneManager::OnRender");

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
    if(activeScene != nullptr) delete activeScene;
    activeScene = new Scene();
    return activeScene;
}

void SceneManager::DestroyActiveScene(){
    delete activeScene;
    activeScene = nullptr;
}

}