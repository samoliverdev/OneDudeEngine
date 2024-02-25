#include "DynamicModule.h"

using namespace OD;

struct DynamicComponent{
    std::string name;
    int age;
    
    template<class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(name));
        ArchiveDump(ar, CEREAL_NVP(age));
    }
};

OD::Module* CreateInstance(){
    return new DynamicModule();
}

void DynamicModule::OnInit(){
    LogInfo("OnInit Dynamic Module");

    Scene* scene = SceneManager::Get().NewScene();
    Entity env = scene->AddEntity("Env");
    env.AddComponent<EnvironmentComponent>().settings.ambient = Vector3(0.11f,0.16f,0.25f);

    /*SceneManager::Get().RegisterCoreComponentSimple<DynamicComponent>("DynamicComponent");

    Scene* scene = SceneManager::Get().GetActiveScene();
    Assert(scene != nullptr);
    Entity e = scene->AddEntity("FromDynamicModule");
    e.AddComponent<DynamicComponent>();*/
}

void DynamicModule::OnExit(){
    LogInfo("OnExit Dynamic Module");
}

void DynamicModule::OnUpdate(float deltaTime){

}
void DynamicModule::OnRender(float deltaTime){

}

void DynamicModule::OnGUI(){}
void DynamicModule::OnResize(int width, int height){}