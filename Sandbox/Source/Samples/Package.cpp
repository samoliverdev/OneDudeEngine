#include "Package.h"
#include "Ultis/CameraMovement.h"
#include "Ultis/Ultis.h"
#include <OD/Core/Application.h>
#include <OD/Core/TarPackage.h>
#include <OD/Scene/SceneManager.h>
#include <OD/RenderPipeline/EnvironmentComponent.h>
#include <OD/RenderPipeline/CameraComponent.h>
#include <OD/RenderPipeline/LightComponent.h>
#include <OD/RenderPipeline/MeshRendererComponent.h>
#include <OD/Graphics/Model.h>
#include <OD/Graphics/Cubemap.h>
#include <OD/Editor/Editor.h>
#include <fstream>

void PackageSample::OnInit(){
    LogInfo("Game Init");
    Application::Vsync(false);

    auto& SceneManager = SceneManager::Get();
    SceneManager.RegisterScript<CameraMovementScript>("CameraMovementScript");
    OD::Scene* scene = SceneManager.NewScene();

    //scene->RemoveSystem<StandRenderPipeline>();
    //scene->AddSystem<DeferredRenderPipeline>();

    Entity camera = scene->AddEntity("Camera");
    CameraComponent& cam = scene->AddComponent<CameraComponent>(camera);
    cam.viewportRect = Vector4(0, 0, 0.5f, 0.5f);
    cam.renderingPath = CameraComponent::RenderingPath::Deferred;
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
    lightComponent.renderShadow = true;
    scene->GetComponent<TransformComponent>(light).Position(Vector3(-2, 4, -1));
    scene->GetComponent<TransformComponent>(light).LocalEulerAngles(Vector3(95, 95, -30));

    Ref<Package> package = CreateRef<TarPackage>("Sandbox/PackageTest.tar");
    
    Ref<Texture2D> tex = Texture2D::CreateFromPackage("image.png", *package, {});
    
    Ref<Model> model = CreateRef<Model>();// AssetManager::Get().LoadAsset<Model>("Engine/Models/Cube.obj");
    //model->LoadFromPackage("Cube.glb", *package);
    model->LoadFromPackage("Model.modelbin", *package);

    Assert(model->meshs.size() > 0);
    
    Entity meshEntity = scene->AddEntity("Mesh");
    MeshRendererComponent& meshRenderer = scene->AddComponent<MeshRendererComponent>(meshEntity);
    meshRenderer.mesh = model->meshs[0];
    meshRenderer.material = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit2.glsl"));
    meshRenderer.material->SetTexture("mainTex", tex);

    Application::AddModule<Editor>();
    //scene->Start();
}

void PackageSample::OnUpdate(float deltaTime){

}

void PackageSample::OnRender(float deltaTime){

}

void PackageSample::OnGUI(){

}

void PackageSample::OnResize(int width, int height){

}

void PackageSample::OnExit(){

}