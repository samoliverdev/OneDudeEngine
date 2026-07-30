#include "OD/pch.h"
#include "RenderPipeline.h"
#include "Ultis/Ultis.h"
#include <OD/Scene/SceneManager.h>
#include <OD/Graphics/Model.h>
#include <OD/Graphics/Shader.h>
#include <OD/Graphics/Cubemap.h>
#include <OD/RenderPipeline/ModelRendererComponent.h>
#include <OD/RenderPipeline/LightComponent.h>
#include <OD/RenderPipeline/CameraComponent.h>
#include <OD/RenderPipeline/EnvironmentComponent.h>
#include <OD/Core/Application.h>
#include <OD/Editor/Editor.h>
#include <assert.h>
//#include <OD/AnimationSystem/Animator.h>

void RenderPipelineSample::AddTransparent(Vector3 pos){
    Assert(SceneManager::Get().GetActiveScene() != nullptr);
    auto& scene = *SceneManager::Get().GetActiveScene();

    Entity et = scene.AddEntity("Transparent");
    scene.GetComponent<TransformComponent>(et).Position(pos);
    scene.GetComponent<TransformComponent>(et).LocalScale(Vector3(10, 10, 10));
    ModelRendererComponent& _meshRenderer3 = scene.AddComponent<ModelRendererComponent>(et);
    _meshRenderer3.SetModel(ResourceManager::Get().LoadByPath<Model>("Engine/Models/plane.obj"));

    for(auto i: _meshRenderer3.GetModel()->materials){
        i->SetShader(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/UnlitBlend.glsl"));
        i->SetTexture("mainTex", ResourceManager::Get().LoadByPath<Texture2D>("Sandbox/Textures/blending_transparent.png"));
    }
}

void RenderPipelineSample::OnInit(){
    //Assert(false && "Not work for now");

    LogInfo("Game Init");
    LogInfo(RESOURCES_PATH "/");

    Application::Vsync(false);

    SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");
    SceneManager::Get().RegisterScript<RotateScript>("RotateScript");

    OD::Ref<OD::Scene> scene = SceneManager::Get().NewScene();
    //scene->RemoveSystem<StandRenderPipeline>();
    //scene->AddSystem<StandRenderPipeline2>();

    std::string defaultShaderPath = "Engine/Shaders/Lit3.glsl";

    Ref<Model> floorModel = ResourceManager::Get().LoadByPath<Model>("Sandbox/Models/plane.glb");
    floorModel->SetShader(ResourceManager::Get().LoadByPath<Shader>(defaultShaderPath));

    Ref<Model> cubeModel = ResourceManager::Get().LoadByPath<Model>("Sandbox/Models/Cube.glb");
    cubeModel->SetShader(ResourceManager::Get().LoadByPath<Shader>(defaultShaderPath));

    Ref<Model> sphereModel = ResourceManager::Get().LoadByPath<Model>("Sandbox/Models/Sphere.glb");
    sphereModel->SetShader(ResourceManager::Get().LoadByPath<Shader>(defaultShaderPath));

    Entity env = scene->AddEntity("Env");
    EnvironmentComponent& envComp = scene->AddComponent<EnvironmentComponent>(env);
    envComp.settings.environmentLight = EnvironmentLight::SkyCubemap;
    /*envComp.settings.toneMappingPostFX->enable = true;
    envComp.settings.toneMappingPostFX->mode = ToneMappingPostFX::Mode::Neutral;
    envComp.settings.colorGradingPostFX->enable = true;
    envComp.settings.colorGradingPostFX->contrast = 18;*/
    //envComp.settings.bloomPostFX->enable = true;
    //envComp.settings.bloomPostFX->intensity = 0.5f;
    envComp.settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};
    envComp.settings.skyCubemap = Cubemap::CreateFromFileHDR("Sandbox/HDRIs/industrial_sunset_puresky_2k.hdr");
    /*envComp.settings.skyCubemap = Cubemap::CreateFromFileHDR("Sandbox/HDRIs/industrial_sunset_puresky_2k.hdr");
    envComp.settings.skyIrradianceMap = Cubemap::CreateIrradianceMapFromCubeMap(envComp.settings.skyCubemap);
    envComp.settings.skyPrefilterMap = Cubemap::CreatePrefilterMapFromCubeMap(envComp.settings.skyCubemap);*/
    //envComp.settings.skyCubemap = envComp.settings.skyIrradianceMap;

    Entity e = scene->AddEntity("Floor");
    scene->GetComponent<TransformComponent>(e).Position(Vector3(0,-2, 0));
    scene->GetComponent<TransformComponent>(e).LocalScale(Vector3(10, 1, 10));
    ModelRendererComponent& _meshRenderer = scene->AddComponent<ModelRendererComponent>(e);
    _meshRenderer.SetModel(floorModel);
    _meshRenderer.GetMaterialsOverride()[0] = LoadFloorMaterial();
    _meshRenderer.GetMaterialsOverride()[0]->SetShader(ResourceManager::Get().LoadByPath<Shader>(defaultShaderPath));

    Entity e2 = scene->AddEntity("Cube");
    scene->GetComponent<TransformComponent>(e2).Position(Vector3(-8, 0, -4));
    scene->GetComponent<TransformComponent>(e2).LocalScale(Vector3(4*1, 4*1, 4*1));
    ModelRendererComponent& _meshRenderer2 = scene->AddComponent<ModelRendererComponent>(e2);
    _meshRenderer2.SetModel(cubeModel);
    _meshRenderer2.GetMaterialsOverride()[0] = LoadFloorMaterial();
    _meshRenderer2.GetMaterialsOverride()[0]->SetShader(ResourceManager::Get().LoadByPath<Shader>(defaultShaderPath));

    /*Entity e3 = scene->AddEntity("Sphere");
    e3.GetComponent<TransformComponent>().Position(Vector3(8, 2, 8));
    e3.GetComponent<TransformComponent>().LocalScale(Vector3(4*1, 4*1, 4*1));
    MeshRendererComponent& _meshRenderer3 = e3.AddComponent<MeshRendererComponent>();
    _meshRenderer3.SetModel(sphereModel);
    _meshRenderer3.GetMaterialsOverride()[0] = LoadFloorMaterial();
    _meshRenderer3.GetMaterialsOverride()[0]->SetShader(AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/Lit.glsl"));
    _meshRenderer3.GetMaterialsOverride()[0]->SetEnableInstancing(true);*/

    ///*

    scene->AddEntityWith<TransformComponent, ModelRendererComponent>("Plane", [&](auto& transform, auto& meshRenderer){
        Ref<Material> m = ResourceManager::Get().Create<Material>();
        m->SetShader(ResourceManager::Get().LoadByPath<Shader>(defaultShaderPath));
        m->SetVector4("color", Vector4(1, 1, 1, 1));
        m->SetTexture("mainTex", ResourceManager::Get().LoadByPath<Texture2D>("Sandbox/Textures/brickwall.jpg"));
        m->SetTexture("normal", ResourceManager::Get().LoadByPath<Texture2D>("Sandbox/Textures/brickwall_normal.jpg"));

        transform.Position(Vector3(-34, 11, 5));
        transform.LocalEulerAngles(Vector3(90, 0, 0));
        transform.LocalScale(Vector3(1, 1, 1));
        meshRenderer.SetModel(floorModel);
        meshRenderer.GetMaterialsOverride()[0] = m;
    });

    scene->AddEntityWith<TransformComponent, ModelRendererComponent>("Sphere", [&](auto& transform, auto& meshRenderer){
        transform.Position(Vector3(8, 2, 8));
        transform.LocalScale(Vector3(4*1, 4*1, 4*1));
        meshRenderer.SetModel(sphereModel);
        meshRenderer.GetMaterialsOverride()[0] = LoadFloorMaterial();
        meshRenderer.GetMaterialsOverride()[0]->SetShader(ResourceManager::Get().LoadByPath<Shader>(defaultShaderPath));
        meshRenderer.GetMaterialsOverride()[0]->SetEnableInstancing(true);
    });

    scene->AddEntityWith<TransformComponent, ModelRendererComponent>("SphereLighting", [&](auto& transform, auto& meshRenderer){
        Ref<Material> material = ResourceManager::Get().Create<Material>();
        *material = *LoadFloorMaterial();
        material->SetTexture("emissionMap", ResourceManager::Get().LoadByPath<Texture2D>("Engine/Textures/White.jpg"));
        material->SetVector4("emissionColor", Vector4(2,2,2,2));

        transform.Position(Vector3(8*2.5f, 2, 8));
        transform.LocalScale(Vector3(4*1, 4*1, 4*1));
        meshRenderer.SetModel(sphereModel);
        meshRenderer.GetMaterialsOverride()[0] = material;
        meshRenderer.GetMaterialsOverride()[0]->SetShader(ResourceManager::Get().LoadByPath<Shader>(defaultShaderPath));
        meshRenderer.GetMaterialsOverride()[0]->SetEnableInstancing(true);
    });

    scene->AddEntityWith<TransformComponent, ModelRendererComponent>("SphereComplexMaterial", [&](auto& transform, auto& meshRenderer){
        Ref<Material> material = ResourceManager::Get().Create<Material>(
            ResourceManager::Get().LoadByPath<Shader>(defaultShaderPath)
        );
        material->SetTexture(
            "mainTex", 
            ResourceManager::Get().LoadByPath<Texture2D>("Sandbox/Materials/Complex/circuitry-albedo.png")
        );
        material->SetTexture(
            "emissionMap", 
            ResourceManager::Get().LoadByPath<Texture2D>("Sandbox/Materials/Complex/circuitry-emission.png")
        );
        material->SetTexture(
            "maskMap", 
            ResourceManager::Get().LoadByPath<Texture2D>("Sandbox/Materials/Complex/circuitry-mask-mods.png")
        );
        material->SetFloat("metallic", 1);
        material->SetEnableInstancing(true);

        transform.Position(Vector3(8*4.5f, 2, 8));
        transform.LocalScale(Vector3(4*1, 4*1, 4*1));
        meshRenderer.SetModel(sphereModel);
        meshRenderer.GetMaterialsOverride()[0] = material;
        //meshRenderer.GetMaterialsOverride()[0]->SetShader(AssetManager::Get().LoadAsset<Shader>(defaultShaderPath));
        //meshRenderer.GetMaterialsOverride()[0]->SetEnableInstancing(true);
    });

    scene->AddEntityWith<TransformComponent, ModelRendererComponent>("SphereMetalic", [&](auto& transform, auto& meshRenderer){
        Ref<Material> material = ResourceManager::Get().Create<Material>(
            ResourceManager::Get().LoadByPath<Shader>(defaultShaderPath)
        );
        //material->SetVector4("color", Vector4(0.52f, 0.82f, 0.56f, 1));
        material->SetFloat("metallic", 1);
        material->SetFloat("smoothness", 0.7f);

        transform.Position(Vector3(8*6.5f, 2, 8));
        transform.LocalScale(Vector3(4*1, 4*1, 4*1));
        meshRenderer.SetModel(sphereModel);
        meshRenderer.GetMaterialsOverride()[0] = material;
    });
    scene->AddEntityWith<TransformComponent, ModelRendererComponent>("SphereMetali2", [&](auto& transform, auto& meshRenderer){
        Ref<Material> material = ResourceManager::Get().Create<Material>(
            ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/Lit2.glsl")
        );
        //material->SetVector4("color", Vector4(0.52f, 0.82f, 0.56f, 1));
        material->SetFloat("metallic", 1);
        material->SetFloat("smoothness", 0.7f);

        transform.Position(Vector3(8*7.5f, 2, 8));
        transform.LocalScale(Vector3(4*1, 4*1, 4*1));
        meshRenderer.SetModel(sphereModel);
        meshRenderer.GetMaterialsOverride()[0] = material;
    });
    //*/

    AddTransparent(Vector3(5, 3, -8));
    AddTransparent(Vector3(6, 3, -12));
    AddTransparent(Vector3(8, 3, -20));

    /*Entity camera2 = scene->AddEntity("Camera2");
    CameraComponent& cam2 = camera2.AddComponent<CameraComponent>();
    cam2.viewportRect = Vector4(0, 0, 0.25f, 0.25f);
    camera2.GetComponent<TransformComponent>().LocalPosition(Vector3(0, 2, 4));
    camera2.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(-25, 0, 0));*/

    Entity camera = scene->AddEntity("Camera");
    CameraComponent& cam = scene->AddComponent<CameraComponent>(camera);
    cam.viewportRect = Vector4(0, 0, 0.5f, 0.5f);
    
    scene->GetComponent<TransformComponent>(camera).LocalPosition(Vector3(0, 3, 8));
    scene->GetComponent<TransformComponent>(camera).LocalEulerAngles(Vector3(-25, 0, 0));
    scene->AddComponent<ScriptComponent>(camera).AddScript<CameraMovementScript>()->moveSpeed = 60;
    //camMove.transform = &camera->GetComponent<TransformComponent>()();
    //camMove.moveSpeed = 60;
    cam.farClipPlane = 1000;

    mainEntity = scene->AddEntity("Main");
    scene->GetComponent<TransformComponent>(mainEntity).LocalPosition(Vector3(2, 0, 0));
    scene->AddComponent<ScriptComponent>(mainEntity).AddScript<RotateScript>();

    ///*
    light = scene->AddEntity("Directional Light");
    LightComponent& lightComponent = scene->AddComponent<LightComponent>(light);
    lightComponent.color = {1,1,1};
    lightComponent.intensity = 2.5f;
    lightComponent.renderShadow = true;
    scene->GetComponent<TransformComponent>(light).Position(Vector3(-2, 4, -1));
    scene->GetComponent<TransformComponent>(light).LocalEulerAngles(Vector3(45, -125, 0));
    //*/

    /*
    Entity light3 = scene->AddEntity("Directional Light");
    LightComponent& lightComponent3 = light3.AddComponent<LightComponent>();
    lightComponent3.color = {1,1,1};
    lightComponent3.intensity = 0.5f;
    lightComponent3.renderShadow = false;
    light3.GetComponent<TransformComponent>().Position(Vector3(-2, 4, -1));
    light3.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(65, -135, 0));
    */

    /*
    Entity light2 = scene->AddEntity("Directional Light 2");
    LightComponent& lightComponent2 = light2.AddComponent<LightComponent>();
    lightComponent2.color = {0.25f, 0.25f, 1};
    lightComponent2.intensity = 1;
    lightComponent2.renderShadow = false;
    light2.GetComponent<TransformComponent>().Position(Vector3(-2, 4, -1));
    light2.GetComponent<TransformComponent>().LocalEulerAngles(-Vector3(45, -125, 0));
    */

    Entity light2 = scene->AddEntity("Spot Light");
    LightComponent& lightComponent2 = scene->AddComponent<LightComponent>(light2);
    lightComponent2.type = LightComponent::Type::Spot;
    lightComponent2.color = {0.25f, 0.25f, 1};
    lightComponent2.intensity = 1000;
    lightComponent2.radius = 100; 
    lightComponent2.coneAngleInner = 80;
    lightComponent2.coneAngleOuter = 85;
    lightComponent2.renderShadow = true;
    scene->GetComponent<TransformComponent>(light2).Position(Vector3(8, 10, 0));
    scene->GetComponent<TransformComponent>(light2).LocalEulerAngles(Vector3(45, 0, 0));

    Entity light3 = scene->AddEntity("Point Light");
    LightComponent& lightComponent3 = scene->AddComponent<LightComponent>(light3);
    lightComponent3.type = LightComponent::Type::Point;
    lightComponent3.color = {1, 0.25f, 0.25f};
    lightComponent3.intensity = 1000;
    lightComponent3.radius = 100; 
    lightComponent3.renderShadow = true;
    scene->GetComponent<TransformComponent>(light3).Position(Vector3(36, 17, 0));
    
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
    for(int i = 0; i < 1; i++){
        float posRange = 200;

        Entity e = scene->AddEntity("Entity" + std::to_string(random(0, 200)));
        scene->AddComponent<ScriptComponent>(e).AddScript<RotateScript>();
        ModelRendererComponent& mr = scene->AddComponent<ModelRendererComponent>(e);
        mr.SetModel(cubeModel);
        mr.GetMaterialsOverride()[0] = LoadFloorMaterial();
        mr.GetMaterialsOverride()[0]->SetEnableInstancing(true);
    
        float angle = 20.0f * i; 
        scene->GetComponent<TransformComponent>(e).LocalPosition(Vector3(random(-posRange, posRange), random(0, posRange), random(-posRange, posRange)));
        scene->GetComponent<TransformComponent>(e).LocalEulerAngles(Vector3(random(-180, 180), random(-180, 180), random(-180, 180)));
        otherEntity = e;

        scene->SetParent(mainEntity, e);
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

void RenderPipelineSample::OnUpdate(float deltaTime){
    //SceneManager::Get().GetActiveScene()->Update();
}   

void RenderPipelineSample::OnRender(float deltaTime){
    //SceneManager::Get().GetActiveScene()->Draw();
}

void RenderPipelineSample::OnGUI(){}
void RenderPipelineSample::OnResize(int width, int height){}
void RenderPipelineSample::OnExit(){}