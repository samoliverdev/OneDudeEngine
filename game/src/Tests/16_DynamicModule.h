#pragma once

#include <OD/OD.h>
#include "Ultis/CameraMovement.h"
#include <assert.h>
#include "Ultis/Ultis.h"
#include <entt/entt.hpp>
#include <fstream>
#include <stdio.h>

using namespace OD;

struct PhysicsCubeS;

inline bool FileExists(const std::string& name){
    std::ifstream f(name.c_str());
    return f.good();
}

struct DynamicModule_16: OD::Module {
    //CameraMovement camMove;
    Entity camera;
    Module* currentModule = nullptr;
    void* currentDll = nullptr;

    #ifdef NDEBUG
    char* modulePath = "./build/Release/dynamic_module.dll";
    #else
    char* modulePath = "build/Debug/dynamic_moduled.dll";
    #endif

    void OnInit() override {
        LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

        /*Scene* scene = SceneManager::Get().NewScene();
        Entity e = scene->AddEntity("Test");*/
        
        SceneManager::Get().RegisterScript<PhysicsCubeS>("PhysicsCubeS");
        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

        Scene* scene = SceneManager::Get().NewScene();

        Entity env = scene->AddEntity("Env");
        env.AddComponent<EnvironmentComponent>().settings.ambient = Vector3(0.11f, 0.16f, 0.25f);

        Entity light = scene->AddEntity("Light");
        LightComponent& lightComponent = light.AddComponent<LightComponent>();
        lightComponent.color = {1,1,1};
        light.GetComponent<TransformComponent>().Position(Vector3(-2, 4, -1));
        light.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(45, -125, 0));
        lightComponent.renderShadow = false;

        camera = scene->AddEntity("Camera");
        CameraComponent& cam = camera.AddComponent<CameraComponent>();
        camera.GetComponent<TransformComponent>().LocalPosition(Vector3(0, 15, 15));
        camera.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(-25, 0, 0));
        camera.AddComponent<ScriptComponent>().AddScript<CameraMovementScript>()->moveSpeed = 60;
        cam.farClipPlane = 1000;

        Ref<Model> floorModel = AssetManager::Get().LoadAsset<Model>("res/Game/Models/plane.glb");
        Ref<Model> cubeModel = AssetManager::Get().LoadAsset<Model>("res/Game/Models/Cube.glb");

        Entity floorEntity = scene->AddEntity("Floor");
        ModelRendererComponent& floorRenderer = floorEntity.AddComponent<ModelRendererComponent>();
        floorRenderer.SetModel(floorModel);
        floorRenderer.GetMaterialsOverride()[0] = LoadFloorMaterial();
        RigidbodyComponent& floorEntityP = floorEntity.AddComponent<RigidbodyComponent>();
        floorEntityP.SetShape(CollisionShape::BoxShape({25,0.1f,25}));
        floorEntityP.Mass(0);
        floorEntityP.SetType(RigidbodyComponent::Type::Static);
        floorEntityP.NeverSleep(true);

        Entity character2Entity = scene->AddEntity("MainCube");
        ModelRendererComponent& character2Renderer = character2Entity.AddComponent<ModelRendererComponent>();
        character2Renderer.SetModel(cubeModel);
        character2Renderer.GetMaterialsOverride()[0] = LoadRockMaterial();
        RigidbodyComponent& physicObject = character2Entity.AddComponent<RigidbodyComponent>();
        physicObject.SetShape(CollisionShape::BoxShape({1,1,1}));
        physicObject.Mass(1);
        physicObject.NeverSleep(true);
        character2Entity.GetComponent<TransformComponent>().Position({2, 13, 0});
        character2Entity.GetComponent<TransformComponent>().Rotation(QuaternionIdentity);

        Entity trigger = scene->AddEntity("Trigger");
        RigidbodyComponent& _trigger = trigger.AddComponent<RigidbodyComponent>();
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

    void OnUpdate(float deltaTime) override {
        //NOTE: HotRelead Test( Not worlking if change class defination)
        if(Input::IsKeyDown(KeyCode::R) && currentModule != nullptr){
            SceneManager::Get().GetActiveScene()->Save("res/tempHotReload.scene");
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
            SceneManager::Get().NewScene()->Load("res/tempHotReload.scene");
        }
    }   

    void OnRender(float deltaTime) override {}
    void OnGUI() override {}
    void OnResize(int width, int height) override {}
    void OnExit() override {}
};