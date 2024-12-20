#include "Game.h"
#include <OD/Defines.h>
#include <OD/Scene/SceneManager.h>
#include <string>


using namespace OD;

struct DynamicComponent{
    std::string name;
    int age;
    float test = 20;
    //float test2 = 26660;
    std::string st = "dfddfd";

    template<class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(name));
        ArchiveDump(ar, CEREAL_NVP(age));
        ArchiveDump(ar, CEREAL_NVP(test));
        //ArchiveDump(ar, CEREAL_NVP(test2));
        ArchiveDump(ar, CEREAL_NVP(st));
    }
};

OD::Module* CreateInstance(){
    return new Game();
}

void GameOnInit(){
    LogInfo("OnInit Dynamic Module");
    LogWarning("1111111111111");

    SceneManager::Get().RegisterCoreComponent<DynamicComponent>("DynamicComponent");

    /*Scene* scene = SceneManager::Get().NewScene();
    Assert(scene != nullptr);
    Entity e = scene->AddEntity("FromDynamicModule");
    e.AddComponent<DynamicComponent>();

    Entity camera = scene->AddEntity("Camera");
    CameraComponent& cam = camera.AddComponent<CameraComponent>();
    camera.GetComponent<TransformComponent>().LocalPosition(Vector3(0, 15, 15));
    camera.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(-25, 0, 0));
    cam.farClipPlane = 1000;*/
}

void GameOnExit(){

}

void GameOnUpdate(){

}

void Game::OnInit(){
    LogInfo("OnInit Dynamic Module");
    LogWarning("llllll");

    /*Scene* scene = SceneManager::Get().NewScene();
    Entity env = scene->AddEntity("Env");
    env.AddComponent<EnvironmentComponent>().settings.ambient = Vector3(0.11f,0.16f,0.25f);*/

    //SceneManager::Get().RegisterCoreComponent<DynamicComponent>("DynamicComponent");

    /*Scene* scene = SceneManager::Get().GetActiveScene();
    Assert(scene != nullptr);
    Entity e = scene->AddEntity("FromDynamicModule");
    e.AddComponent<DynamicComponent>();*/
}

void Game::OnExit(){
    LogInfo("OnExit Dynamic Module");
}

void Game::OnUpdate(float deltaTime){
    //LogInfo("OnUpdate Dynamic Module");
}

void Game::OnRender(float deltaTime){
    //LogInfo("OnRender Dynamic Module");
}

void Game::OnGUI(){}
void Game::OnResize(int width, int height){}