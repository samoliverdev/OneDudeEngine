#pragma once

#include <OD/OD.h>
#include "Ultis/CameraMovement.h"
#include <assert.h>
#include "Ultis/Ultis.h"
#include <entt/entt.hpp>

using namespace OD;

struct PlayerController: public Script{
    float moveSpeed = 1000;
    float turnSpeed = 20;

    Clip* idleAnimation;
    Clip* runningAnimation;

    Ref<AudioClip> shootClip;

    void OnStart() override{
        Assert(idleAnimation != nullptr);
        Assert(runningAnimation != nullptr);

        shootClip = AssetManager::Get().LoadAsset<AudioClip>("res/Game/Sounds/633250__aesterial-arts__arcade-shoot.wav");
        AudioSourceComponent& audioSource = GetEntity().AddComponent<AudioSourceComponent>();
        audioSource.clip = shootClip;
    }

    void OnUpdate() override{
        Vector3 moveDir = Vector3Zero;
        RigidbodyComponent& rb = GetEntity().GetComponent<RigidbodyComponent>();
        AnimatorComponent& anim = GetEntity().GetComponent<AnimatorComponent>();

        if(Input::IsKey(KeyCode::W)) moveDir += Vector3Back;
        if(Input::IsKey(KeyCode::S)) moveDir += Vector3Forward;
        if(Input::IsKey(KeyCode::A)) moveDir += Vector3Left;
        if(Input::IsKey(KeyCode::D)) moveDir += Vector3Right;
        if(math::length(moveDir) > 1) moveDir = math::normalize(moveDir);

        Assert(Mathf::IsNan(moveDir) == false);

        rb.Velocity(moveDir * moveSpeed * Application::DeltaTime());
        if(moveDir != Vector3Zero){
            rb.Rotation(
                math::slerp(rb.Rotation(), math::quatLookAt(-moveDir, Vector3Up), turnSpeed * Application::DeltaTime())
            );
        }

        if(moveDir == Vector3Zero){
            if(anim.controller.GetCurrentClip() != idleAnimation) anim.FadeTo(idleAnimation, 0.1f);
        } else {
            if(anim.controller.GetCurrentClip() != runningAnimation) anim.FadeTo(runningAnimation, 0.1f);
        }

        AudioSourceComponent& audioSource = GetEntity().GetComponent<AudioSourceComponent>();
        if(Input::IsKeyDown(KeyCode::Space)){
            audioSource.Play();
        }
    }

    void OnDestroy() override{}

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, moveSpeed);
        ArchiveDumpNVP(ar, turnSpeed);
    }
};

struct CharacterControllerSample: OD::Module {
    Entity camera;
    Navmesh navmesh;

    void OnInit() override {
        LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

        Application::Vsync(true);

        SceneManager::Get().RegisterScript<PhysicsCubeS>("PhysicsCubeS");
        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");
        SceneManager::Get().RegisterScript<PlayerController>("PlayerController");

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

        Entity e2 = scene->AddEntity("Cube");
        e2.GetComponent<TransformComponent>().Position(Vector3(8, 0, 4));
        e2.GetComponent<TransformComponent>().LocalScale(Vector3(4*1, 4*1, 4*1));
        ModelRendererComponent& _meshRenderer2 = e2.AddComponent<ModelRendererComponent>();
        Assert(cubeModel != nullptr); 
        _meshRenderer2.SetModel(cubeModel);
        Assert(_meshRenderer2.GetMaterialsOverride().size() > 0);
        _meshRenderer2.GetMaterialsOverride()[0] = LoadFloorMaterial();
        
        Entity e3 = scene->AddEntity("Cube2");
        e3.GetComponent<TransformComponent>().Position(Vector3(-8, 0, -4));
        e3.GetComponent<TransformComponent>().LocalScale(Vector3(4*1, 4*1, 4*1));
        ModelRendererComponent& _meshRenderer3 = e3.AddComponent<ModelRendererComponent>();
        Assert(cubeModel != nullptr); 
        _meshRenderer3.SetModel(cubeModel);
        Assert(_meshRenderer2.GetMaterialsOverride().size() > 0);
        _meshRenderer3.GetMaterialsOverride()[0] = LoadFloorMaterial();

        Ref<Model> charIdleModel = AssetManager::Get().LoadAsset<Model>("res/Game/Animations/Idle.dae");
        Ref<Model> charRunningModel = AssetManager::Get().LoadAsset<Model>("res/Game/Animations/Running.dae");
        charIdleModel->SetShader(AssetManager::Get().LoadAsset<Shader>("res/Engine/Shaders/Lit.glsl"));

        Entity playerEntity = scene->AddEntity("PlayerController");
        TransformComponent& charTrans = playerEntity.GetComponent<TransformComponent>();
        //charTrans.LocalScale(Vector3(0.01f));
        SkinnedModelRendererComponent& charRenderer = playerEntity.AddComponent<SkinnedModelRendererComponent>();
        charRenderer.localTransform.LocalScale(Vector3(200));
        charRenderer.skeletonTransform.LocalScale(Vector3(0.01f));
        charRenderer.SetModel(charIdleModel);
        charRenderer.SetAABB(Vector3(0,0.01f,0), Vector3(0.01f/2, 0.01f, 0.01f/4));
        charRenderer.UpdatePosePalette();
        RigidbodyComponent& rb = playerEntity.AddComponent<RigidbodyComponent>();
        rb.NeverSleep(true);
        rb.SetShape(CollisionShape::CapsuleShape(0.5f, 2.5f, Vector3(0, 1.25f, 0)));
        PlayerController* playerController = playerEntity.AddComponent<ScriptComponent>().AddScript<PlayerController>();
        playerController->idleAnimation = charIdleModel->animationClips[0].get();
        playerController->runningAnimation = charRunningModel->animationClips[0].get();
        //rb.SetType(RigidbodyComponent::Type::Kinematic);
        LogInfo("CharModel Skeleton RestPose Size: %d", charIdleModel->skeleton.GetRestPose().Size());
        Assert(charRenderer.posePalette.size() == charIdleModel->skeleton.GetRestPose().Size());
        AnimatorComponent& charAnim = playerEntity.AddComponent<AnimatorComponent>();
        charAnim.Play(charIdleModel->animationClips[0].get());
        
        /*Entity navmeshEntity = scene->AddEntity("Navmesh");
        NavmeshComponent& navmeshComp = navmeshEntity.AddComponent<NavmeshComponent>();
        navmeshComp.navmesh = CreateRef<Navmesh>();
        navmeshComp.navmesh->buildSettings.cellSize = 0.3f;
        navmeshComp.navmesh->buildSettings.cellHeight = 0.2f;
        navmeshComp.navmesh->buildSettings.agentHeight = 2.0f;
        navmeshComp.navmesh->buildSettings.agentRadius = 0.6f;
        navmeshComp.navmesh->buildSettings.agentMaxClimb = 0.9f;
        navmeshComp.navmesh->buildSettings.agentMaxSlope = 45.0f;
        navmeshComp.navmesh->buildSettings.regionMinSize = 8;
        navmeshComp.navmesh->buildSettings.regionMergeSize = 20;
        navmeshComp.navmesh->buildSettings.edgeMaxLen = 12.0f;
        navmeshComp.navmesh->buildSettings.edgeMaxError = 1.3f;
        navmeshComp.navmesh->buildSettings.vertsPerPoly = 6.0f;
        navmeshComp.navmesh->buildSettings.detailSampleDist = 6.0f;
        navmeshComp.navmesh->buildSettings.detailSampleMaxError = 1.0f;
        navmeshComp.navmesh->buildSettings.partitionType = SAMPLE_PARTITION_WATERSHED;
        navmeshComp.navmesh->Bake(scene, AABB(Vector3(-15.5f, 0, -12.5f), Vector3(100, 100, 100)));*/

        /*navmesh.buildSettings.cellSize = 0.3f;
        navmesh.buildSettings.cellHeight = 0.2f;
        navmesh.buildSettings.agentHeight = 2.0f;
        navmesh.buildSettings.agentRadius = 0.6f;
        navmesh.buildSettings.agentMaxClimb = 0.9f;
        navmesh.buildSettings.agentMaxSlope = 45.0f;
        navmesh.buildSettings.regionMinSize = 8;
        navmesh.buildSettings.regionMergeSize = 20;
        navmesh.buildSettings.edgeMaxLen = 12.0f;
        navmesh.buildSettings.edgeMaxError = 1.3f;
        navmesh.buildSettings.vertsPerPoly = 6.0f;
        navmesh.buildSettings.detailSampleDist = 6.0f;
        navmesh.buildSettings.detailSampleMaxError = 1.0f;
        navmesh.buildSettings.partitionType = SAMPLE_PARTITION_WATERSHED;
        navmesh.Bake(scene, AABB(Vector3Zero, Vector3One * 200.0f));*/
        
        //scene->Start();
        Application::AddModule<Editor>();
    }

    void OnUpdate(float deltaTime) override {}   
    void OnRender(float deltaTime) override {}
    void OnGUI() override {}
    void OnResize(int width, int height) override {}
    void OnExit() override {}
};