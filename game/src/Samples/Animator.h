#pragma once

#include <OD/OD.h>
#include "Ultis/CameraMovement.h"
#include <assert.h>
#include "Ultis/Ultis.h"
#include <entt/entt.hpp>

using namespace OD;

struct AnimatorSample: OD::Module {
    Entity camera;
    //std::vector<FastClip> clips;

    void OnInit() override {
        LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

        Application::Vsync(false);

        SceneManager::Get().RegisterScript<PhysicsCubeS>("PhysicsCubeS");
        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

        Scene* scene = SceneManager::Get().NewScene();

        Entity env = scene->AddEntity("Env");
        env.AddComponent<EnvironmentComponent>().settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};

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
        TransformComponent& floorTransform = floorEntity.GetComponent<TransformComponent>();
        floorTransform.LocalScale(Vector3(5));
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
            //"res/Game/Animations/Walking.dae"
            "res/Game/Animations/RumbaDancing3.glb"
            //"res/Game/Animations/SillyDancing.fbx"
            //"res/Game/Animations/UnarmedWalkForward.dae"
        );
        charModel->SetShader(AssetManager::Get().LoadAsset<Shader>("res/Engine/Shaders/Lit.glsl"));

        
        /*OD::BoneMap bm = OD::RearrangeSkeleton(charModel->skeleton);
        //OD::RearrangeMesh(*charModel->meshs[0], bm);
        for(auto i: charModel->animationClips){
            clips.push_back(OD::OptimizeClip(*i));
            OD::RearrangeFastclip(clips[clips.size()-1], bm);
        }*/
        
        /*Entity charEntity = scene->AddEntity("Character");
        TransformComponent& charTrans = charEntity.GetComponent<TransformComponent>();
        charTrans.LocalScale(Vector3(0.01f));
        SkinnedModelRendererComponent& charRenderer = charEntity.AddComponent<SkinnedModelRendererComponent>();
        charRenderer.SetModel(charModel);
        //charRenderer.CreateSkeletonEntites(*scene, charEntity);
        charRenderer.SetAABB(Vector3(0,0.01f,0), Vector3(0.01f/2, 0.01f, 0.01f/4));
        charRenderer.UpdatePosePalette();
        LogInfo("CharModel Skeleton RestPose Size: %d", charModel->skeleton.GetRestPose().Size());
        Assert(charRenderer.posePalette.size() == charModel->skeleton.GetRestPose().Size());
        AnimatorComponent& charAnim = charEntity.AddComponent<AnimatorComponent>();
        charAnim.Play(charModel->animationClips[0].get());*/
        
        const int Size = 3;
        for(int x = -(Size/2); x <= (Size/2); x++){
            for(int y = -(Size/2); y <= (Size/2); y++){
                Entity charEntity = scene->AddEntity("Character");
                
                TransformComponent& charTrans = charEntity.GetComponent<TransformComponent>();
                charTrans.Position(Vector3(x*2, 0, y*2));
                charTrans.LocalScale(Vector3(1));
                
                SkinnedModelRendererComponent& charRenderer = charEntity.AddComponent<SkinnedModelRendererComponent>();
                charRenderer.SetModel(charModel);
                charRenderer.SetAABB(Vector3(0,0.01f,0), Vector3(0.01f/2, 0.01f, 0.01f/4));
                charRenderer.UpdatePosePalette();
                LogInfo("CharModel Skeleton RestPose Size: %d", charModel->skeleton.GetRestPose().Size());
                Assert(charRenderer.posePalette.size() == charModel->skeleton.GetRestPose().Size());
                
                AnimatorComponent& charAnim = charEntity.AddComponent<AnimatorComponent>();
                charAnim.Play(charModel->animationClips[0].get() /*&clips[0]*/);
            }
        }
        
        //scene->Start();
        RenderContext::GetSettings().enableGizmos = false;
        Application::AddModule<Editor>();

        LogInfo("AnimationCount: %zd", charModel->animationClips.size());
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