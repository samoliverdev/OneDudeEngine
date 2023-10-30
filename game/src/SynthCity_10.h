#pragma once

#include <OD/OD.h>
#include "CameraMovement.h"
#include <assert.h>
#include "Ultis.h"

using namespace OD;

struct SynthCity_10: OD::Module {
    //CameraMovement camMove;
    Entity camera;

    void OnInit() override {
        LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

        Application::vsync(false);

        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

        Scene* scene = SceneManager::Get().NewScene();

        Entity env = scene->AddEntity("Env");
        env.AddComponent<EnvironmentComponent>().settings.ambient = Vector3(0.11f,0.16f,0.25f);

        Entity light = scene->AddEntity("Light");
        LightComponent& lightComponent = light.AddComponent<LightComponent>();
        lightComponent.color = Vector3(1,1,1);
        light.GetComponent<TransformComponent>().position(Vector3(-2, 4, -1));
        light.GetComponent<TransformComponent>().localEulerAngles(Vector3(45, -125, 0));
        lightComponent.renderShadow = false;

        camera = scene->AddEntity("Camera");
        CameraComponent& cam = camera.AddComponent<CameraComponent>();
        camera.GetComponent<TransformComponent>().localPosition(Vector3(0, 15, 15));
        camera.GetComponent<TransformComponent>().localEulerAngles(Vector3(-25, 0, 0));
        camera.AddComponent<TestC>();
        camera.AddComponent<ScriptComponent>().AddScript<CameraMovementScript>()->moveSpeed = 60;
        cam.farClipPlane = 1000;

        Ref<Model> floorModel = AssetManager::Get().LoadModel(
            "res/PolygonCity/FBX_SCENE/City.fbx",
            AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/StandDiffuse.glsl")
        );
  
        Entity floorEntity = scene->AddEntity("City");
        MeshRendererComponent& floorRenderer = floorEntity.AddComponent<MeshRendererComponent>();
        floorRenderer.model(floorModel);
        TransformComponent& cityTransform = floorEntity.GetComponent<TransformComponent>();
        cityTransform.localScale(Vector3(0.01f, 0.01f, 0.01f));
    
        Application::AddModule<Editor>();
        //scene->Start();
    }

    void OnUpdate(float deltaTime) override {
        SceneManager::Get().activeScene()->Update();
    }   

    void OnRender(float deltaTime) override {
        SceneManager::Get().activeScene()->Draw();
    }

    void OnGUI() override {}
    void OnResize(int width, int height) override {}
};