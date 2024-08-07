#pragma once

#include <OD/OD.h>
#include "Ultis/CameraMovement.h"
#include <assert.h>
#include "Ultis/Ultis.h"
#include <fstream>

using namespace OD;

const float boundsSize = 200; 

struct BoidComponent{
    Vector3 velocity = Vector3Zero;

    //static inline void OnGui(Entity& e){}

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, velocity);
    }
};

struct BoidSystem: public OD::System{
    float boidPerceptionRadius = 5;
    float boidSpeed = 25;
    
    float avoidBoundsTurnDist = 5;
    float avoidBoundsWeight = 10;

    float separationWeight = 25;
    float coheshionWeight = 5;
    float alignmentWeight = 10;

    BoidSystem(Scene* inScene):System(inScene){}
    System* Clone(Scene* inScene) const override{ return new BoidSystem(inScene); }

    void Update() override{
        auto boidsView = scene->GetRegistry().view<TransformComponent, BoidComponent>();
        for(auto e: boidsView){
            TransformComponent& trans = boidsView.get<TransformComponent>(e);
            BoidComponent& boid = boidsView.get<BoidComponent>(e);

            Vector3 separationSum = Vector3Zero;
            Vector3 coheshionSum = Vector3Zero;
            Vector3 alignmentSum = Vector3Zero;
            int count = 0;

            for(auto e2: boidsView){
                if(e == e2) continue;

                TransformComponent& otherTrans = boidsView.get<TransformComponent>(e2);
                BoidComponent& otherBoid = boidsView.get<BoidComponent>(e2);
                
                float distanceFromOther = math::distance(trans.LocalPosition(), otherTrans.LocalPosition());
                if(distanceFromOther > boidPerceptionRadius || trans.LocalPosition() == otherTrans.LocalPosition()) continue;

                separationSum += (trans.LocalPosition() - otherTrans.LocalPosition()) / distanceFromOther;
                coheshionSum += otherTrans.LocalPosition();
                alignmentSum += otherBoid.velocity;
                count += 1;
            }

            Vector3 velocity = Vector3Zero;
            if(count > 0){
                velocity += (separationSum / (float)count) * separationWeight;
                velocity += ((coheshionSum / (float)count) - trans.LocalPosition()) * coheshionWeight;
                velocity += (alignmentSum / (float)count) * alignmentWeight;
            }
            if(math::min(math::min(
                (boundsSize) - math::abs(trans.LocalPosition().x),
                (boundsSize) - math::abs(trans.LocalPosition().y)),
                (boundsSize) - math::abs(trans.LocalPosition().z))
                    < avoidBoundsTurnDist){
                velocity += -math::normalize(trans.LocalPosition()) * avoidBoundsWeight;
            }

            boid.velocity = math::normalize(boid.velocity + velocity);
            trans.LocalPosition(trans.LocalPosition() + boid.velocity * boidSpeed * Application::DeltaTime());
            trans.LocalRotation(math::quatLookAt(-boid.velocity, Vector3Up));
        }
    }

    void OnDrawGizmos() override{
        Transform trans;
        trans.LocalScale(Vector3(boundsSize));
        Graphics::DrawWireCube(trans.GetLocalModelMatrix(), Vector3(0, 1, 0), 1);
    }
};

struct BoidsSample: public OD::Module {

    void OnInit() override {
        LogInfo("Game Init");
        Application::Vsync(false);

        auto& SceneManager = SceneManager::Get();
        SceneManager.RegisterScript<CameraMovementScript>("CameraMovementScript");
        SceneManager.RegisterComponent<BoidComponent>("BoidComponent");
        SceneManager.RegisterSystem<BoidSystem>("BoidSystem");
        OD::Scene* scene = SceneManager.NewScene();

        Ref<Material> floorMaterial = CreateRef<Material>();
        floorMaterial->SetShader(AssetManager::Get().LoadAsset<Shader>("res/Engine/Shaders/Lit.glsl"));
        floorMaterial->SetVector4("color", Vector4(0.8f, 0.8f, 0.8f, 1));

        Ref<Material> boidMaterial = CreateRef<Material>();
        boidMaterial->SetShader(AssetManager::Get().LoadAsset<Shader>("res/Engine/Shaders/Lit.glsl"));
        boidMaterial->SetVector4("color", Vector4(1, 0, 0, 1));
        boidMaterial->SetEnableInstancing(true);

        Ref<Model> floorModel = AssetManager::Get().LoadAsset<Model>("res/Game/Models/plane.glb");
        floorModel->SetShader(AssetManager::Get().LoadAsset<Shader>("res/Engine/Shaders/Lit.glsl"));

        Ref<Model> boidModel = AssetManager::Get().LoadAsset<Model>("res/Game/Models/Boid.glb");
        boidModel->SetShader(AssetManager::Get().LoadAsset<Shader>("res/Engine/Shaders/Lit.glsl"));

        Entity env = scene->AddEntity("Env");
        env.AddComponent<EnvironmentComponent>().settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};

        Entity e = scene->AddEntity("Floor");
        e.GetComponent<TransformComponent>().Position(Vector3(0, -(boundsSize/2), 0));
        e.GetComponent<TransformComponent>().LocalScale(Vector3(15, 1, 15));
        ModelRendererComponent& _meshRenderer = e.AddComponent<ModelRendererComponent>();
        _meshRenderer.SetModel(floorModel);
        _meshRenderer.GetMaterialsOverride()[0] = floorMaterial;

        Entity camera = scene->AddEntity("Camera");
        CameraComponent& cam = camera.AddComponent<CameraComponent>();
        cam.viewportRect = Vector4(0, 0, 0.5f, 0.5f);
        camera.GetComponent<TransformComponent>().LocalPosition(Vector3(0, 150, 150));
        camera.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(-25, 0, 0));
        camera.AddComponent<ScriptComponent>().AddScript<CameraMovementScript>()->moveSpeed = 60;
        //camMove.transform = &camera->GetComponent<TransformComponent>()();
        //camMove.moveSpeed = 60;
        cam.farClipPlane = 1000;

        Entity light = scene->AddEntity("Directional Light");
        LightComponent& lightComponent = light.AddComponent<LightComponent>();
        lightComponent.color = {1,1,1};
        lightComponent.intensity = 1.5f;
        lightComponent.renderShadow = true;
        light.GetComponent<TransformComponent>().Position(Vector3(-2, 4, -1));
        light.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(45, -125, 0));

        float posRange = 200;
        Entity boids = scene->AddEntity("Boids");
        
        for(int i = 0; i < 1000; i++){
            Entity boid = scene->AddEntity("Boid" + std::to_string(random(0, 200)));
            ModelRendererComponent& mr = boid.AddComponent<ModelRendererComponent>();
            mr.SetModel(boidModel);
            mr.GetMaterialsOverride()[0] = boidMaterial;

            TransformComponent& trans = boid.GetComponent<TransformComponent>();
            trans.LocalPosition(Vector3(random(-posRange, posRange), random(-posRange, posRange), random(-posRange, posRange)));
            trans.LocalEulerAngles(Vector3(random(-180, 180), random(-180, 180), random(-180, 180)));

            BoidComponent& boidComponet = boid.AddComponent<BoidComponent>();
            boidComponet.velocity = trans.Forward();
        
            scene->SetParent(boids, boid);
        }

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