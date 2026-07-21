#include "OD/pch.h"
#include "AssetPacking.h"
#include "Ultis/CameraMovement.h"
#include "Ultis/Ultis.h"
#include <OD/Core/Application.h>
#include <OD/Scene/SceneManager.h>
#include <OD/RenderPipeline/EnvironmentComponent.h>
#include <OD/RenderPipeline/CameraComponent.h>
#include <OD/RenderPipeline/LightComponent.h>
#include <OD/RenderPipeline/MeshRendererComponent.h>
#include <OD/RenderPipeline/ModelRendererComponent.h>
#include <OD/Animation/Animator.h>
#include <OD/Graphics/Model.h>
#include <OD/Graphics/Cubemap.h>
#include <OD/Graphics/Texture.h>
#include <OD/Editor/Editor.h>
#include <OD/Editor/ExtraPanels/BuildsPanel.h>

void AssetPackingSample::OnInit(){
    LogInfo("Game Init");
    Application::Vsync(false);

    auto& SceneManager = SceneManager::Get();
    SceneManager.RegisterScript<CameraMovementScript>("CameraMovementScript");
    OD::Ref<OD::Scene> scene = SceneManager.NewScene();

    //scene->RemoveSystem<StandRenderPipeline>();
    //scene->AddSystem<DeferredRenderPipeline>();

    {
        AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/water.png");
    }

    Entity camera = scene->AddEntity("Camera");
    CameraComponent& cam = scene->AddComponent<CameraComponent>(camera);
    cam.viewportRect = Vector4(0, 0, 0.5f, 0.5f);
    cam.renderingPath = RenderingPath::Deferred;
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

    #if 0
    //Ref<Model> model = AssetManager::Get().LoadAsset<Model>("Engine/Models/Cube.obj");
    Ref<Model> model = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/Sponza/sponza.glb");
    model->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl"));

    model = AssetManager::Get().LoadAsset<Model>("Standard/Models/YBot.glb");
    model->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl"));
    
    //model->meshs[0]->Save("Sandbox/Mesh.meshasset", Asset::SaveType::AssetBinary);
    //model->meshs[0]->Save("Sandbox/Mesh.meshbin", Asset::SaveType::FinalBinary);

    //model->Save("Sandbox/Model.modelasset", Asset::SaveType::AssetBinary);
    model->Save("Sandbox/Model.modelbin", Asset::SaveType::FinalBinary);

    Ref<Model> modelBin = AssetManager::Get().LoadAsset<Model>("Sandbox/Model.modelbin");
    
    /*Assert(modelBin != nullptr);
    scene->Instantiate(modelBin);*/

    Entity modelEntity = scene->AddEntity("Model");
    SkinnedModelRendererComponent& renderer = scene->AddComponent<SkinnedModelRendererComponent>(modelEntity);
    renderer.SetModel(modelBin);
    //renderer.SetModel(model);
    AnimatorComponent& anim = scene->AddComponent<AnimatorComponent>(modelEntity);

    /*Ref<Mesh> meshBin = AssetManager::Get().LoadAsset<Mesh>("Sandbox/Mesh.meshbin");
    Assert(meshBin != nullptr);
    Entity meshEntity = scene->AddEntity("Mesh");
    MeshRendererComponent& meshRenderer = scene->AddComponent<MeshRendererComponent>(meshEntity);
    meshRenderer.mesh = meshBin;
    meshRenderer.material = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit2.glsl"));*/
    #else

    //Ref<Shader> shader = AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl");
    //shader->Save("Sandbox/lit.shaderbin", Asset::SaveType::FinalBinary);

    Ref<Shader> shader2 = AssetManager::Get().LoadAsset<Shader>("Sandbox/Lit");

    Ref<Model> model = AssetManager::Get().LoadAsset<Model>("Engine/Models/Cube.obj");

    //Ref<Texture2D> texture = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/image.png");
    //texture->Save("Sandbox/image.texturebin", Asset::SaveType::FinalBinary);

    Ref<Material> mat = CreateRef<Material>(shader2);

    Ref<Texture2D> textureBin = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/image.texturebin");
    Assert(textureBin != nullptr);

    //mat->SetTexture("mainTex", texture);
    mat->SetTexture("mainTex", textureBin);

    Entity modelEntity = scene->AddEntity("Model");
    ModelRendererComponent& renderer = scene->AddComponent<ModelRendererComponent>(modelEntity);
    renderer.SetModel(model);
    for(auto& i: renderer.GetMaterialsOverride()) i = mat;

    #endif

    Application::AddModule<Editor>();

    buildsPanel = CreateRef<BuildsPanel>();
    buildsPanel->buildPath = "C:/Users/sam/Desktop/BuildTest/Content/";
    Editor* editor = Application::GetModuleByType<Editor>();
    editor->AddCustomPanel(buildsPanel.get());
    //scene->Start();
}

void AssetPackingSample::OnUpdate(float deltaTime){

}

void AssetPackingSample::OnRender(float deltaTime){

}

void AssetPackingSample::OnGUI(){

}

void AssetPackingSample::OnResize(int width, int height){

}

void AssetPackingSample::OnExit(){

}