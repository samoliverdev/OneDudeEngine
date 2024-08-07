#pragma once

#include <OD/OD.h>
#include "Ultis/CameraMovement.h"
#include <assert.h>
#include "Ultis/Ultis.h"
#include <fstream>

using namespace OD;

struct SponzaSample: public OD::Module {

    void OnInit() override {
        LogInfo("Game Init");
        Application::Vsync(false);

        auto& SceneManager = SceneManager::Get();
        SceneManager.RegisterScript<CameraMovementScript>("CameraMovementScript");
        OD::Scene* scene = SceneManager.NewScene();

        Ref<Model> sponzaModel = AssetManager::Get().LoadAsset<Model>("res/Game/Models/Sponza/sponza.glb");
        sponzaModel->SetShader(AssetManager::Get().LoadAsset<Shader>("res/Engine/Shaders/Lit.glsl"));

        Entity env = scene->AddEntity("Env");
        EnvironmentComponent& envComp = env.AddComponent<EnvironmentComponent>();
        envComp.settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};
        envComp.settings.toneMappingPostFX->enable = true;
        envComp.settings.toneMappingPostFX->mode = ToneMappingPostFX::Mode::ACES;

        /*Entity e = scene->AddEntity("Sponza");
        //e.GetComponent<TransformComponent>().LocalScale(Vector3(0.01f));
        ModelRendererComponent& _meshRenderer = e.AddComponent<ModelRendererComponent>();
        _meshRenderer.SetModel(sponzaModel);*/

        Entity e = scene->Instantiate(sponzaModel);
        //e.GetComponent<TransformComponent>().LocalScale(Vector3(0.01f));

        Ref<Model> cubeModel = AssetManager::Get().LoadAsset<Model>("res/Game/Models/Cube.glb");
        cubeModel->SetShader(AssetManager::Get().LoadAsset<Shader>("res/Engine/Shaders/Lit.glsl"));

        /*Entity cube = scene->AddEntity("CubeRef");
        ModelRendererComponent& _meshRenderer2 = cube.AddComponent<ModelRendererComponent>();
        _meshRenderer2.SetModel(cubeModel);*/

        Entity camera = scene->AddEntity("Camera");
        CameraComponent& cam = camera.AddComponent<CameraComponent>();
        cam.viewportRect = Vector4(0, 0, 0.5f, 0.5f);
        camera.GetComponent<TransformComponent>().LocalPosition(Vector3(7, 2.5, 0));
        camera.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(-8, 90, 0));
        camera.AddComponent<ScriptComponent>().AddScript<CameraMovementScript>()->moveSpeed = 10;
        //camMove.transform = &camera->GetComponent<TransformComponent>()();
        //camMove.moveSpeed = 60;
        cam.farClipPlane = 1000;

        Entity light = scene->AddEntity("Directional Light");
        LightComponent& lightComponent = light.AddComponent<LightComponent>();
        lightComponent.color = {1,1,1};
        lightComponent.intensity = 1.5f;
        lightComponent.renderShadow = true;
        light.GetComponent<TransformComponent>().Position(Vector3(-2, 4, -1));
        light.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(95, 95, -30));

        Entity pointLight = scene->AddEntity("Point Light");
        LightComponent& lightComponent2 = pointLight.AddComponent<LightComponent>();
        lightComponent2.color = {1,1,0.8f};
        lightComponent2.type = LightComponent::Type::Point;
        lightComponent2.intensity = 5.0f;
        lightComponent2.radius = 100.0f;
        lightComponent2.renderShadow = true;
        pointLight.GetComponent<TransformComponent>().Position(Vector3(0, 4, 0));

        Entity pointLight2 = scene->AddEntity("Point Light 2");
        LightComponent& lightComponent3 = pointLight2.AddComponent<LightComponent>();
        lightComponent3.color = {1,1,0.8f};
        lightComponent3.type = LightComponent::Type::Point;
        lightComponent3.intensity = 3;
        lightComponent3.radius = 5;
        lightComponent3.renderShadow = false;
        pointLight2.GetComponent<TransformComponent>().Position(Vector3(3, 0.02f, 0));

        Application::AddModule<Editor>();
        scene->Start();
    }

    void OnUpdate(float deltaTime) override {
        //SceneManager::Get().GetActiveScene()->Update();
    }   

    void OnRender(float deltaTime) override {
        //SceneManager::Get().GetActiveScene()->Draw();
    }

    void OnGUI() override {}
    void OnResize(int width, int height) override{}
    void OnExit() override {}

};