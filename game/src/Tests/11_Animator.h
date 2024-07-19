#pragma once

#include <OD/OD.h>
#include "Ultis/CameraMovement.h"
#include <assert.h>
#include "Ultis/Ultis.h"
#include <entt/entt.hpp>

using namespace OD;

struct Animator_11: OD::Module {
    Entity camera;

    void OnInit() override {
        LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

        Application::Vsync(false);

        SceneManager::Get().RegisterScript<PhysicsCubeS>("PhysicsCubeS");
        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

        Scene* scene = SceneManager::Get().NewScene();

        Entity env = scene->AddEntity("Env");
        env.AddComponent<EnvironmentComponent>().settings.ambient = Vector3(0.11f,0.16f,0.25f);

        Entity light = scene->AddEntity("Light");
        LightComponent& lightComponent = light.AddComponent<LightComponent>();
        lightComponent.color = {1,1,1};
        light.GetComponent<TransformComponent>().Position(Vector3(-2, 4, -1));
        light.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(45, -125, 0));

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
        //floorEntityP->entity()->transform().localEulerAngles({0,0,-25});

        Ref<Model> charModel = AssetManager::Get().LoadAsset<Model>(
            "res/Game/Animations/Walking.dae"
            //"res/Game/Animations/RumbaDancing.glb"
        );
        charModel->SetShader(AssetManager::Get().LoadAsset<Shader>("res/Engine/Shaders/Lit.glsl"));

        Entity charEntity = scene->AddEntity("Character");
        TransformComponent& charTrans = charEntity.GetComponent<TransformComponent>();
        charTrans.LocalScale(Vector3(200));
        SkinnedModelRendererComponent& charRenderer = charEntity.AddComponent<SkinnedModelRendererComponent>();
        charRenderer.SetModel(charModel);
        charRenderer.SetAABB(Vector3(0,0.01f,0), Vector3(0.01f/2, 0.01f, 0.01f/4));
        charRenderer.UpdatePosePalette();
        LogInfo("CharModel Skeleton RestPose Size: %d", charModel->skeleton.GetRestPose().Size());
        Assert(charRenderer.posePalette.size() == charModel->skeleton.GetRestPose().Size());
        AnimatorComponent& charAnim = charEntity.AddComponent<AnimatorComponent>();
        charAnim.Play(charModel->animationClips[0].get());
        
        //scene->Start();
        Application::AddModule<Editor>();

        //LogInfo("BonesCount: %zd", charModel->skeleton.GetRestPose().Size());
    }

    void OnUpdate(float deltaTime) override {
        //Scene* scene = SceneManager::Get().GetActiveScene();
        //scene->Update();
        //if(scene->Running() == false) return;
    }   

    void OnRender(float deltaTime) override {
        //SceneManager::Get().GetActiveScene()->Draw();
    }

    void OnGUI() override {}
    void OnResize(int width, int height) override {}
    void OnExit() override {}
};