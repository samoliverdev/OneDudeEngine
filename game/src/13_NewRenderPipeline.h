#pragma once

#include <OD/OD.h>
//#include <OD/AnimationSystem/Animator.h>
#include "CameraMovement.h"
#include <assert.h>
#include "Ultis.h"
#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_speech.h>
#include <soloud_thread.h>
#include <thread>
#include <future>
#include <fstream>
#include <OD/RenderPipeline/StandRenderPipeline2.h>

using namespace OD;

struct NewRenderPipeline_13: public OD::Module {
    //CameraMovement camMove;

    Entity mainEntity;
    Entity camera;
    Entity light;
    Entity otherEntity;

    Transform gismoTransform;

    std::future<void> playMusic;

    SoLoud::Soloud soloud;
    SoLoud::Wav sample;

    void AddTransparent(Vector3 pos){
        Entity et = SceneManager::Get().GetActiveScene()->AddEntity("Transparent");
        et.GetComponent<TransformComponent>().Position(pos);
        et.GetComponent<TransformComponent>().LocalScale(Vector3(10, 10, 10));
        MeshRendererComponent& _meshRenderer3 = et.AddComponent<MeshRendererComponent>();
        _meshRenderer3.SetModel(AssetManager::Get().LoadModel("res/Engine/Models/plane.obj"));

        for(auto i: _meshRenderer3.GetModel()->materials){
            i->SetShader(AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/UnlitBlend.glsl"));
            i->SetTexture("mainTex", AssetManager::Get().LoadTexture2D("res/Engine/Textures/blending_transparent.png", {TextureFilter::Linear, true}));
        }
    }

    void OnInit() override {
        LogInfo("Game Init");
        LogInfo(RESOURCES_PATH "/");

        Application::Vsync(false);

        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");
        SceneManager::Get().RegisterScript<RotateScript>("RotateScript");

        OD::Scene* scene = SceneManager::Get().NewScene();
        //scene->RemoveSystem<StandRenderPipeline>();
        //scene->AddSystem<StandRenderPipeline2>();

        Ref<Model> floorModel = AssetManager::Get().LoadModel(
            "res/Game/Models/plane.glb",
            AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/Lit.glsl")
        );

        Ref<Model> cubeModel = AssetManager::Get().LoadModel(
            "res/Game/Models/Cube.glb",
            AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/Lit.glsl")
        );

        Ref<Model> sphereModel = AssetManager::Get().LoadModel(
            "res/Game/Models/Sphere.glb",
            AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/Lit.glsl")
        );

        Entity env = scene->AddEntity("Env");
        env.AddComponent<EnvironmentComponent>().settings.ambient = Vector3(0.11f,0.16f,0.25f);

        Entity e = scene->AddEntity("Floor");
        e.GetComponent<TransformComponent>().Position(Vector3(0,-2, 0));
        e.GetComponent<TransformComponent>().LocalScale(Vector3(10, 1, 10));
        MeshRendererComponent& _meshRenderer = e.AddComponent<MeshRendererComponent>();
        _meshRenderer.SetModel(floorModel);
        _meshRenderer.GetMaterialsOverride()[0] = LoadFloorMaterial();
        _meshRenderer.GetMaterialsOverride()[0]->SetShader(AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/Lit.glsl"));
   
        Entity e2 = scene->AddEntity("Cube");
        e2.GetComponent<TransformComponent>().Position(Vector3(-8, 0, -4));
        e2.GetComponent<TransformComponent>().LocalScale(Vector3(4*1, 4*1, 4*1));
        MeshRendererComponent& _meshRenderer2 = e2.AddComponent<MeshRendererComponent>();
        _meshRenderer2.SetModel(cubeModel);
        _meshRenderer2.GetMaterialsOverride()[0] = LoadFloorMaterial();
        _meshRenderer2.GetMaterialsOverride()[0]->SetShader(AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/Lit.glsl"));

        Entity e3 = scene->AddEntity("Sphere");
        e3.GetComponent<TransformComponent>().Position(Vector3(8, 2, 8));
        e3.GetComponent<TransformComponent>().LocalScale(Vector3(4*1, 4*1, 4*1));
        MeshRendererComponent& _meshRenderer3 = e3.AddComponent<MeshRendererComponent>();
        _meshRenderer3.SetModel(sphereModel);
        _meshRenderer3.GetMaterialsOverride()[0] = LoadFloorMaterial();
        _meshRenderer3.GetMaterialsOverride()[0]->SetShader(AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/Lit.glsl"));

        AddTransparent(Vector3(5, 3, -8));
        AddTransparent(Vector3(6, 3, -12));
        AddTransparent(Vector3(8, 3, -20));

        /*Entity camera2 = scene->AddEntity("Camera2");
        CameraComponent& cam2 = camera2.AddComponent<CameraComponent>();
        cam2.viewportRect = Vector4(0, 0, 0.25f, 0.25f);
        camera2.GetComponent<TransformComponent>().LocalPosition(Vector3(0, 2, 4));
        camera2.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(-25, 0, 0));*/

        Entity camera = scene->AddEntity("Camera");
        CameraComponent& cam = camera.AddComponent<CameraComponent>();
        cam.viewportRect = Vector4(0, 0, 0.5f, 0.5f);
        
        camera.GetComponent<TransformComponent>().LocalPosition(Vector3(0, 2, 4));
        camera.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(-25, 0, 0));
        camera.AddComponent<ScriptComponent>().AddScript<CameraMovementScript>()->moveSpeed = 60;
        //camMove.transform = &camera->GetComponent<TransformComponent>()();
        //camMove.moveSpeed = 60;
        cam.farClipPlane = 1000;

        mainEntity = scene->AddEntity("Main");
        mainEntity.GetComponent<TransformComponent>().LocalPosition(Vector3(2, 0, 0));
        mainEntity.AddComponent<ScriptComponent>().AddScript<RotateScript>();

        ///*
        light = scene->AddEntity("Directional Light");
        LightComponent& lightComponent = light.AddComponent<LightComponent>();
        lightComponent.color = Vector3(1,1,1);
        lightComponent.intensity = 1.5f;
        lightComponent.renderShadow = true;
        light.GetComponent<TransformComponent>().Position(Vector3(-2, 4, -1));
        light.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(45, -125, 0));
        //*/

        /*
        Entity light3 = scene->AddEntity("Directional Light");
        LightComponent& lightComponent3 = light3.AddComponent<LightComponent>();
        lightComponent3.color = Vector3(1,1,1);
        lightComponent3.intensity = 0.5f;
        lightComponent3.renderShadow = false;
        light3.GetComponent<TransformComponent>().Position(Vector3(-2, 4, -1));
        light3.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(65, -135, 0));
        */

        /*
        Entity light2 = scene->AddEntity("Directional Light 2");
        LightComponent& lightComponent2 = light2.AddComponent<LightComponent>();
        lightComponent2.color = Vector3(0.25f, 0.25f, 1);
        lightComponent2.intensity = 1;
        lightComponent2.renderShadow = false;
        light2.GetComponent<TransformComponent>().Position(Vector3(-2, 4, -1));
        light2.GetComponent<TransformComponent>().LocalEulerAngles(-Vector3(45, -125, 0));
        */
        
        /*
        Entity pointLight = scene->AddEntity("Point Light");
        LightComponent& _pointLight = pointLight.AddComponent<LightComponent>();
        _pointLight.type = LightComponent::Type::Point;
        _pointLight.color = Vector3(1,1,1);
        _pointLight.intensity = 5;
        _pointLight.radius = 10;
        pointLight.GetComponent<TransformComponent>().Position(Vector3(4, 4, 0));

        Entity pointLight2 = scene->AddEntity("Point Light 2");
        LightComponent& _pointLight2 = pointLight2.AddComponent<LightComponent>();
        _pointLight2.type = LightComponent::Type::Point;
        _pointLight2.color = Vector3(0,0,1);
        _pointLight2.intensity = 5;
        _pointLight2.radius = 20;
        pointLight2.GetComponent<TransformComponent>().Position(Vector3(-3, 0.5f, 0));
        */

        ///*
        for(int i = 0; i < 10; i++){
            float posRange = 200;

            Entity e = scene->AddEntity("Entity" + std::to_string(random(0, 200)));
            e.AddComponent<ScriptComponent>().AddScript<RotateScript>();
            MeshRendererComponent& mr = e.AddComponent<MeshRendererComponent>();
            mr.SetModel(cubeModel);
            mr.GetMaterialsOverride()[0] = LoadFloorMaterial();
        
            float angle = 20.0f * i; 
            e.GetComponent<TransformComponent>().LocalPosition(Vector3(random(-posRange, posRange), random(0, posRange), random(-posRange, posRange)));
            e.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(random(-180, 180), random(-180, 180), random(-180, 180)));
            otherEntity = e;

            scene->SetParent(mainEntity.Id(), e.Id());
        }
        //*/

        /*Ref<Model> charModel = AssetManager::Get().LoadModel(
            "res/Game/Animations/Walking.dae",
            AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/Lit.glsl")
        );
        Entity charEntity = scene->AddEntity("Character");
        TransformComponent& charTrans = charEntity.GetComponent<TransformComponent>();
        charTrans.LocalScale(Vector3(200.0f, 200.0f, 200.0f));
        SkinnedMeshRendererComponent& charRenderer = charEntity.AddComponent<SkinnedMeshRendererComponent>();
        charRenderer.SetModel(charModel);
        //charRenderer.SetDefaultAABB();
        charRenderer.UpdatePosePalette();
        LogInfo("CharModel Skeleton RestPose Size: %d", charModel->skeleton.GetRestPose().Size());
        Assert(charRenderer.posePalette.size() == charModel->skeleton.GetRestPose().Size());
        AnimatorComponent& charAnim = charEntity.AddComponent<AnimatorComponent>();
        charAnim.Play(charModel->animationClips[0].get());
        */

        Application::AddModule<Editor>();
        //scene->Start();
    }

    void OnUpdate(float deltaTime) override {
        SceneManager::Get().GetActiveScene()->Update();
    }   

    void OnRender(float deltaTime) override {
        SceneManager::Get().GetActiveScene()->Draw();
    }

    void OnGUI() override {}
    void OnResize(int width, int height) override{}
    void OnExit() override {}

};