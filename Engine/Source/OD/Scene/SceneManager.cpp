#include "OD/pch.h"
#include "SceneManager.h"
#include "Prefab.h"
#include "EntityHandle.h"
#include "OD/Core/Application.h"
#include "OD/Core/Lua.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Core/GlobalSettings.h"
#include "OD/Serialization/CerealImGui.h"

namespace OD{

void SceneManagerModuleInit(){
    Application::AddModule(&SceneManager::Get());

    LuaBindsDB::Get().RegisterLuaBind<TransformComponent>();
    LuaBindsDB::Get().RegisterLuaBind<InfoComponent>();
    LuaBindsDB::Get().RegisterLuaBind<EntityHandle>();
    LuaBindsDB::Get().RegisterLuaBind<Scene>();
    
    SceneManager::Get().RegisterComponent<SelfDisable>("SelfDisable");
    SceneManager::Get().RegisterComponent<SkipDraw>("SkipDraw");
    SceneManager::Get().RegisterComponent<DontSave>("DontSave");
    SceneManager::Get().RegisterComponent<HideInEditor>("HideInEditor");

    AssetTypesDB::Get().RegisterAssetType<Prefab>(".prefab", 
        [](const std::string& path){ return AssetManager::Get().LoadAsset<Prefab>(path); }
    );

    GlobalSettings::Get().Register<GlobalSceneData>("Layers");
}

SceneManager& SceneManager::Get(){
    static SceneManager sceneManager;
    return sceneManager;
}

void SceneManager::OnInit(){}

void SceneManager::OnExit(){
    if(activeScene != nullptr){
        //delete activeScene;
        activeScene = nullptr;
    }

    for(auto& i : globalSystems){
        delete i.second;
    };
    globalSystems.clear();

    coreComponentsSerializer.clear();
    componentsSerializer.clear();
    scriptsSerializer.clear();
    addSystemFuncs.clear();
}

void SceneManager::OnUpdate(float deltaTime){
    OD_PROFILE_SCOPE("SceneManager::OnUpdate");

    if(toLoad.empty() == false){
        Ref<Scene> scene = NewScene();
        scene->Load(toLoad.c_str());
        scene->Start();

        toLoad = "";
        tempScene = nullptr;
        toLoadTempScene = false;
        isLoading = false;
        tempScene = nullptr;
    }

    //Info: Temp, remove this later
    if(toLoadTempScene){
        if(tempSceneIsSave){
            tempScene = activeScene;
            activeScene = CreateRef<Scene>(*tempScene);
            activeScene->Start();

            toLoad = "";
            isLoading = false;
            toLoadTempScene = false;
        } else {
            activeScene = tempScene;
            activeScene->Start();

            toLoad = "";
            isLoading = false;
            toLoadTempScene = false;
            tempScene = nullptr;
        }
    }

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

//Info: Temp, remove this later
void SceneManager::_LoadTempClonedScene(bool isSave){
    toLoadTempScene = true;
    tempSceneIsSave = isSave;
    isLoading = true;
}

bool SceneManager::_HasTempScene(){
    return tempScene != nullptr;
}

void SceneManager::LoadScene(const std::string& path){ 
    toLoad = path; 
    isLoading = true;
}

bool SceneManager::IsLoading(){ 
    return isLoading; 
}

SceneManager::SceneState SceneManager::GetSceneState(){ 
    return sceneState; 
}

bool SceneManager::InEditor(){ 
    return inEditor; 
}

Ref<Scene> SceneManager::GetActiveScene(){ 
    //if(activeScene == nullptr) return NewScene();
    return activeScene; 
}

void SceneManager::SetActiveScene(Ref<Scene> s){ 
    activeScene = s; 
}

Ref<Scene> SceneManager::NewScene(){
    //if(activeScene != nullptr) delete activeScene;
    activeScene = CreateRef<Scene>();// new Scene();
    return activeScene;
}

void SceneManager::DestroyActiveScene(){
    //delete activeScene;
    activeScene = nullptr;
}

}