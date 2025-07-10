#include "DynamicModule.h"
#include "Physics.h"
#include "Ultis/CameraMovement.h"
#include "Ultis/Ultis.h"
#include <OD/Core/Application.h>
#include <OD/Core/Input.h>
#include <OD/Scene/Scripts.h>
#include <OD/Scene/SceneManager.h>
#include <OD/RenderPipeline/LightComponent.h>
#include <OD/RenderPipeline/CameraComponent.h>
#include <OD/RenderPipeline/ModelRendererComponent.h>
#include <OD/RenderPipeline/EnvironmentComponent.h>
#include <OD/Physics/PhysicsSystem.h>
#include <OD/Editor/Editor.h>
#include <assert.h>
#include <fstream>
#include <stdio.h>
#include <entt/entt.hpp>

inline bool FileExists(const std::string& name){
    std::ifstream f(name.c_str());
    return f.good();
}

#ifdef NDEBUG
const char* modulePath = "../build/Debug/DynamicModule.dll";
#else
const char* modulePath = "../build/Release/DynamicModule.dll";
#endif

void DynamicModuleSample::OnInit(){
    LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

    /*Scene* scene = SceneManager::Get().NewScene();
    Entity e = scene->AddEntity("Test");*/
    
    SceneManager::Get().RegisterScript<PhysicsCubeS>("PhysicsCubeS");
    SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

    Scene* scene = SceneManager::Get().NewScene();

    Entity env = scene->AddEntity("Env");
    scene->AddComponent<EnvironmentComponent>(env).settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};

    Entity light = scene->AddEntity("Light");
    LightComponent& lightComponent = scene->AddComponent<LightComponent>(light);
    lightComponent.color = {1,1,1};
    scene->GetComponent<TransformComponent>(light).Position(Vector3(-2, 4, -1));
    scene->GetComponent<TransformComponent>(light).LocalEulerAngles(Vector3(45, -125, 0));
    lightComponent.renderShadow = false;

    camera = scene->AddEntity("Camera");
    CameraComponent& cam = scene->AddComponent<CameraComponent>(camera);
    scene->GetComponent<TransformComponent>(camera).LocalPosition(Vector3(0, 15, 15));
    scene->GetComponent<TransformComponent>(camera).LocalEulerAngles(Vector3(-25, 0, 0));
    scene->AddComponent<ScriptComponent>(camera).AddScript<CameraMovementScript>()->moveSpeed = 60;
    cam.farClipPlane = 1000;

    Ref<Model> floorModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/plane.glb");
    Ref<Model> cubeModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/Cube.glb");

    Entity floorEntity = scene->AddEntity("Floor");
    ModelRendererComponent& floorRenderer = scene->AddComponent<ModelRendererComponent>(floorEntity);
    floorRenderer.SetModel(floorModel);
    floorRenderer.GetMaterialsOverride()[0] = LoadFloorMaterial();
    RigidbodyComponent& floorEntityP = scene->AddComponent<RigidbodyComponent>(floorEntity);
    floorEntityP.SetShape(CollisionShape::BoxShape({25,0.1f,25}));
    floorEntityP.Mass(0);
    floorEntityP.SetType(RigidbodyComponent::Type::Static);
    floorEntityP.NeverSleep(true);

    Entity character2Entity = scene->AddEntity("MainCube");
    ModelRendererComponent& character2Renderer = scene->AddComponent<ModelRendererComponent>(character2Entity);
    character2Renderer.SetModel(cubeModel);
    character2Renderer.GetMaterialsOverride()[0] = LoadRockMaterial();
    RigidbodyComponent& physicObject = scene->AddComponent<RigidbodyComponent>(character2Entity);
    physicObject.SetShape(CollisionShape::BoxShape({1,1,1}));
    physicObject.Mass(1);
    physicObject.NeverSleep(true);
    scene->GetComponent<TransformComponent>(character2Entity).Position({2, 13, 0});
    scene->GetComponent<TransformComponent>(character2Entity).Rotation(QuaternionIdentity);

    Entity trigger = scene->AddEntity("Trigger");
    RigidbodyComponent& _trigger = scene->AddComponent<RigidbodyComponent>(trigger);
    _trigger.SetShape(CollisionShape::BoxShape({4,1,4}));
    _trigger.SetType(RigidbodyComponent::Type::Trigger);
    _trigger.NeverSleep(true);
    

    //scene->Save("res/scene1.scene");
    //scene->Start();
    Application::AddModule<Editor>();

    
    if(FileExists(modulePath)){
        typedef Module* (*CreateInstanceFunc)();
        currentDll = Platform::LoadDynamicLibrary(modulePath);
        CreateInstanceFunc func = (CreateInstanceFunc)Platform::LoadDynamicFunction(currentDll, "CreateInstance");
        currentModule = func();
        Application::AddModule(currentModule);
    } else {
        LogError("Load Dynamic Module");
    }
}

void DynamicModuleSample::OnUpdate(float deltaTime){
    //NOTE: HotRelead Test( Not worlking if change class defination)
    if(Input::IsKeyDown(KeyCode::R) && currentModule != nullptr){
        SceneManager::Get().GetActiveScene()->Save("tempHotReload.scene", EntityNull);
        //Clean Old Refs
        Application::RemoveModule(currentModule);
        Platform::FreeDynimicLibrary(currentDll);

        //system("cmake -S . -B build");
        #ifdef NDEBUG
        system("cmake --build build --config Release");
        #else 
        system("cmake --build build --config Debug");
        #endif

        typedef Module* (*CreateInstanceFunc)();
        currentDll = Platform::LoadDynamicLibrary(modulePath);
        CreateInstanceFunc func = (CreateInstanceFunc)Platform::LoadDynamicFunction(currentDll, "CreateInstance");
        currentModule = func();
        Application::AddModule(currentModule);
        SceneManager::Get().NewScene()->Load("tempHotReload.scene");
    }
}   

void DynamicModuleSample::OnRender(float deltaTime){}
void DynamicModuleSample::OnGUI(){}
void DynamicModuleSample::OnResize(int width, int height){}
void DynamicModuleSample::OnExit(){}