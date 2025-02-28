#include "Sponza.h"
#include "Ultis/CameraMovement.h"
#include <assert.h>
#include "Ultis/Ultis.h"
#include <fstream>
//#include <OD/RenderPipeline/DeferredRenderPipeline.h>

void SponzaSample::OnInit(){
    LogInfo("Game Init");
    Application::Vsync(false);

    auto& SceneManager = SceneManager::Get();
    SceneManager.RegisterScript<CameraMovementScript>("CameraMovementScript");
    OD::Scene* scene = SceneManager.NewScene();

    //scene->RemoveSystem<StandRenderPipeline>();
    //scene->AddSystem<DeferredRenderPipeline>();

    Ref<Model> sponzaModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/Sponza/sponza.glb");
    sponzaModel->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Unlit.glsl"));

    Entity env = scene->AddEntity("Env");
    EnvironmentComponent& envComp = scene->AddComponent<EnvironmentComponent>(env);
    envComp.settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};
    //envComp.settings.toneMappingPostFX->enable = true;
    //envComp.settings.toneMappingPostFX->mode = ToneMappingPostFX::Mode::ACES;

    /*Entity e = scene->AddEntity("Sponza");
    //e.GetComponent<TransformComponent>().LocalScale(Vector3(0.01f));
    ModelRendererComponent& _meshRenderer = e.AddComponent<ModelRendererComponent>();
    _meshRenderer.SetModel(sponzaModel);*/

    Entity e = scene->Instantiate(sponzaModel);
    //e.GetComponent<TransformComponent>().LocalScale(Vector3(0.01f));

    Ref<Model> cubeModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/Cube.glb");
    cubeModel->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Unlit.glsl"));

    /*Entity cube = scene->AddEntity("CubeRef");
    ModelRendererComponent& _meshRenderer2 = cube.AddComponent<ModelRendererComponent>();
    _meshRenderer2.SetModel(cubeModel);*/

    Entity camera = scene->AddEntity("Camera");
    CameraComponent& cam = scene->AddComponent<CameraComponent>(camera);
    cam.viewportRect = Vector4(0, 0, 0.5f, 0.5f);
    //cam.renderingPath = CameraComponent::RenderingPath::Deferred;
    scene->GetComponent<TransformComponent>(camera).LocalPosition(Vector3(7, 2.5, 0));
    scene->GetComponent<TransformComponent>(camera).LocalEulerAngles(Vector3(-8, 90, 0));
    scene->AddComponent<ScriptComponent>(camera).AddScript<CameraMovementScript>()->moveSpeed = 10;
    //camMove.transform = &camera->GetComponent<TransformComponent>()();
    //camMove.moveSpeed = 60;
    cam.farClipPlane = 1000;

    Entity light = scene->AddEntity("Directional Light");
    LightComponent& lightComponent = scene->AddComponent<LightComponent>(light);
    lightComponent.color = {1,1,1};
    lightComponent.intensity = 1.5f;
    lightComponent.renderShadow = false;
    scene->GetComponent<TransformComponent>(light).Position(Vector3(-2, 4, -1));
    scene->GetComponent<TransformComponent>(light).LocalEulerAngles(Vector3(95, 95, -30));

    Entity pointLight = scene->AddEntity("Point Light");
    LightComponent& lightComponent2 = scene->AddComponent<LightComponent>(pointLight);
    lightComponent2.color = {1,1,0.8f};
    lightComponent2.type = LightComponent::Type::Point;
    lightComponent2.intensity = 5.0f;
    lightComponent2.radius = 100.0f;
    lightComponent2.renderShadow = false;
    scene->GetComponent<TransformComponent>(pointLight).Position(Vector3(0, 4, 0));

    Entity pointLight2 = scene->AddEntity("Point Light 2");
    LightComponent& lightComponent3 = scene->AddComponent<LightComponent>(pointLight2);
    lightComponent3.color = {1,1,0.8f};
    lightComponent3.type = LightComponent::Type::Point;
    lightComponent3.intensity = 3;
    lightComponent3.radius = 5;
    lightComponent3.renderShadow = false;
    scene->GetComponent<TransformComponent>(pointLight2).Position(Vector3(3, 0.02f, 0));

    //Application::AddModule<Editor>();
    scene->Start();
}

void SponzaSample::OnUpdate(float deltaTime){
    //SceneManager::Get().GetActiveScene()->Update();
}   

void SponzaSample::OnRender(float deltaTime){
    //SceneManager::Get().GetActiveScene()->Draw();
}

void SponzaSample::OnGUI(){}
void SponzaSample::OnResize(int width, int height){}
void SponzaSample::OnExit(){}