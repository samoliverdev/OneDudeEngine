#include "Boids.h"
#include "Ultis/CameraMovement.h"
#include "Ultis/Ultis.h"
#include <OD/Scene/Scene.h>
#include <OD/Scene/SceneManager.h>
#include <OD/Graphics/Graphics.h>
#include <OD/RenderPipeline/EnvironmentComponent.h>
#include <OD/RenderPipeline/ModelRendererComponent.h>
#include <OD/RenderPipeline/CameraComponent.h>
#include <OD/RenderPipeline/LightComponent.h>
#include <OD/Core/Application.h>
#include <OD/Core/Instrumentor.h>
#include <OD/Editor/Editor.h>
#include <fstream>

#undef max
#undef min

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
    //System* Clone(Scene* inScene) const override{ return new BoidSystem(inScene); }

    template<typename T>
    size_t get_index(T* base, T* ptr) {
        return static_cast<size_t>(ptr - base);
    }

    void Update() override{
        OD_PROFILE_SCOPE("BoidSystem::Update");

        //entt::view<entt::get_t<TransformComponent, BoidComponent>> boidsView = scene->GetRegistry().view<TransformComponent, BoidComponent>();
        //auto test = std::make_shared<entt::view<entt::get_t<TransformComponent, BoidComponent>>>(boidsView);

        auto boidsView = scene->GetRegistry().group<TransformComponent, BoidComponent>();
        
        auto firstTrans = &std::get<1>(*boidsView.each().begin());
        auto firstBoid  = &std::get<2>(*boidsView.each().begin());

        for(auto e: boidsView){
            TransformComponent& trans = boidsView.get<TransformComponent>(e);
            BoidComponent& boid = boidsView.get<BoidComponent>(e);

            size_t transIndex = get_index(firstTrans, &trans);
            size_t boidIndex  = get_index(firstBoid, &boid);
            LogInfo("Entity index: %zd, Boid index: %zd", transIndex, boidIndex);

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
            if(math::min(math::min(//math::min
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

    void OnDrawGizmos(Camera& cam) override{
        Transform trans;
        trans.Scale(Vector3(boundsSize));
        Graphics::DrawWireCube(trans.GetModelMatrix(), Vector3(0, 1, 0), 1);
    }
};


void BoidsSample::OnInit(){
    LogInfo("Game Init");
    Application::Vsync(false);

    auto& SceneManager = SceneManager::Get();
    SceneManager.RegisterScript<CameraMovementScript>("CameraMovementScript");
    SceneManager.RegisterComponent<BoidComponent>("BoidComponent");
    SceneManager.RegisterSystem<BoidSystem>("BoidSystem");
    OD::Scene* scene = SceneManager.NewScene();

    Ref<Material> floorMaterial = CreateRef<Material>();
    floorMaterial->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl"));
    floorMaterial->SetVector4("color", Vector4(0.8f, 0.8f, 0.8f, 1));

    Ref<Material> boidMaterial = CreateRef<Material>();
    boidMaterial->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl"));
    boidMaterial->SetVector4("color", Vector4(1, 0, 0, 1));
    boidMaterial->SetEnableInstancing(true);

    Ref<Model> floorModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/plane.glb");
    floorModel->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl"));

    Ref<Model> boidModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/Boid.glb");
    boidModel->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl"));

    Entity env = scene->AddEntity("Env");
    scene->AddComponent<EnvironmentComponent>(env).settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};

    Entity e = scene->AddEntity("Floor");
    scene->GetComponent<TransformComponent>(e).Position(Vector3(0, -(boundsSize/2), 0));
    scene->GetComponent<TransformComponent>(e).LocalScale(Vector3(15, 1, 15));
    ModelRendererComponent& _meshRenderer = scene->AddComponent<ModelRendererComponent>(e);
    _meshRenderer.SetModel(floorModel);
    _meshRenderer.GetMaterialsOverride()[0] = floorMaterial;

    Entity camera = scene->AddEntity("Camera");
    CameraComponent& cam = scene->AddComponent<CameraComponent>(camera);
    cam.viewportRect = Vector4(0, 0, 0.5f, 0.5f);
    scene->GetComponent<TransformComponent>(camera).LocalPosition(Vector3(0, 150, 150));
    scene->GetComponent<TransformComponent>(camera).LocalEulerAngles(Vector3(-25, 0, 0));
    scene->AddComponent<ScriptComponent>(camera).AddScript<CameraMovementScript>()->moveSpeed = 60;
    //camMove.transform = &camera->GetComponent<TransformComponent>()();
    //camMove.moveSpeed = 60;
    cam.farClipPlane = 1000;

    Entity light = scene->AddEntity("Directional Light");
    LightComponent& lightComponent = scene->AddComponent<LightComponent>(light);
    lightComponent.color = {1,1,1};
    lightComponent.intensity = 1.5f;
    lightComponent.renderShadow = true;
    scene->GetComponent<TransformComponent>(light).Position(Vector3(-2, 4, -1));
    scene->GetComponent<TransformComponent>(light).LocalEulerAngles(Vector3(45, -125, 0));

    float posRange = 200;
    Entity boids = scene->AddEntity("Boids");
    
    for(int i = 0; i < 1000; i++){
        Entity boid = scene->AddEntity("Boid" + std::to_string(random(0, 200)));
        ModelRendererComponent& mr = scene->AddComponent<ModelRendererComponent>(boid);
        mr.SetModel(boidModel);
        mr.GetMaterialsOverride()[0] = boidMaterial;

        TransformComponent& trans = scene->GetComponent<TransformComponent>(boid);
        trans.LocalPosition(Vector3(random(-posRange, posRange), random(-posRange, posRange), random(-posRange, posRange)));
        trans.LocalEulerAngles(Vector3(random(-180, 180), random(-180, 180), random(-180, 180)));

        //if(i % 2 == 0) continue;
        if(i % 2 == 0) scene->AddComponent<SkinnedModelRendererComponent>(boid);

        BoidComponent& boidComponet = scene->AddComponent<BoidComponent>(boid);
        boidComponet.velocity = trans.Forward();
    
        scene->SetParent(boids, boid);
    }

    Application::AddModule<Editor>();
    scene->Start();
}

void BoidsSample::OnUpdate(float deltaTime){
    //SceneManager::Get().GetActiveScene()->Update();
}   

void BoidsSample::OnRender(float deltaTime){
    //SceneManager::Get().GetActiveScene()->Draw();
}

void BoidsSample::OnGUI(){}
void BoidsSample::OnResize(int width, int height){}
void BoidsSample::OnExit(){}