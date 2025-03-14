#include "Physics.h"
#include <assert.h>
#include "Ultis/Ultis.h"
#include <entt/entt.hpp>
#include <sol/sol.hpp>
#include <OD/LuaScripting/LuaScripts.h>
#include "Ultis/CameraMovement.h"

void PhysicsCubeS::OnStart(){
    sol::state lua;
    int x = 0;
    lua.set_function("beep", [&x]{ ++x; });
    lua.script("beep()");
    Assert(x == 1);

    LogInfo("PhysicsCubeS OnStart");
    
    Assert(scene->IsValid(entity) == true);

    Ref<Model> cubeModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/Cube.obj");
    //cubeModel->SetShader(AssetManager::GetGlobal()->LoadShaderFromFile("res/Builtins/Shaders/Unlit.glsl"));
    //cubeModel->materials[0].SetTexture("mainTex", AssetManager::GetGlobal()->LoadTexture2D("res/textures/rock.jpg", false, OD::TextureFilter::Linear, false));
    //cubeModel->materials[0].SetVector4("color", Vector4(1, 1, 1, 1));
    cubeModel->materials[0] = LoadMaterial1();

    ModelRendererComponent& renderer = scene->AddOrGetComponent<ModelRendererComponent>(entity);
    renderer.SetModel(cubeModel);

    RigidbodyComponent& physicObject = scene->AddOrGetComponent<RigidbodyComponent>(entity);
    physicObject.NeverSleep(true);
    
    //physicObject->boxShapeSize = {1,1,1};
    //physicObject->mass = 1;

    physicObject.SetShape(CollisionShape::BoxShape({1,1,1}));
    //physicObject->SetMass(1);
}

void PhysicsCubeS::OnUpdate(){
    ///*
    t += Application::DeltaTime();
    if(t > timeToDestroy){
        scene->DestroyEntity(entity);
        //LogInfo("ToDestroy");
    }
    //*/
}

void PhysicsCubeS::OnDestroy(){
    LogInfo("PhysicsCubeS OnDestroy");
}

void PhysicsSample::OnInit(){
    LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

    SceneManager::Get().RegisterScript<PhysicsCubeS>("PhysicsCubeS");
    SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

    Scene* scene = SceneManager::Get().NewScene();

    /*Entity text = scene->AddEntity("Text");
    scene->GetComponent<TransformComponent>(text).LocalPosition(Vector3(25.0f, 25.0f, 0));
    TextRendererComponent& textRenderer = scene->AddComponent<TextRendererComponent>(text);
    textRenderer.text = "Ai meu cu!!!";
    textRenderer.color = {0.5f, 0.8f, 0.2f, 1.0f};
    textRenderer.font = Font::CreateFromFile("Engine/Fonts/OpenSans/static/OpenSans_Condensed-Bold.ttf");
    textRenderer.material = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Font.glsl"));*/

    /*Entity sprite = scene->AddEntity("Sprite");
    scene->GetComponent<TransformComponent>(sprite).LocalPosition(Vector3(0, 2, 0));
    SpriteRendererComponent& spriteRenderer = scene->AddComponent<SpriteRendererComponent>(sprite);
    spriteRenderer.sprite = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/character_1.png"); 
    spriteRenderer.color = {0.5f, 0.8f, 0.2f, 1.0f};
    //spriteRenderer.texture = CreateRef<Texture2D>("Sandbox/Textures/character_1.png", Texture2DSetting()); // Erro 
    spriteRenderer.material = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Sprite.glsl"));

    Entity canvas = scene->AddEntity("Canvas");
    CanvasComponent& can = scene->AddComponent<CanvasComponent>(canvas);

    Entity uiImage = scene->AddEntity("UIImage");
    RectTransformComponet& rect = scene->AddComponent<RectTransformComponet>(uiImage);
    rect.pos = {250, 250};
    rect.size = Vector2(300, 200);
    rect.anchors = Vector2(-1, -1);
    UIImageComponent& uiImageRenderer = scene->AddComponent<UIImageComponent>(uiImage);
    uiImageRenderer.sourceImage = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/image.jpg"); 
    uiImageRenderer.material = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Sprite.glsl"));
    scene->SetParent(canvas, uiImage);

    Entity uiImage2 = scene->AddEntity("UIImage2");
    RectTransformComponet& rect2 = scene->AddComponent<RectTransformComponet>(uiImage2);
    rect2.size = Vector2(100, 100);
    rect2.anchors = Vector2(1, 1);
    UIImageComponent& uiImageRenderer2 = scene->AddComponent<UIImageComponent>(uiImage2);
    uiImageRenderer2.sourceImage = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/block.png"); 
    uiImageRenderer2.material = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Sprite.glsl"));
    scene->SetParent(uiImage, uiImage2);

    Entity uiImage3 = scene->AddEntity("UIImage3");
    RectTransformComponet& rect3 = scene->AddComponent<RectTransformComponet>(uiImage3);
    rect3.pos = {25, 25};
    rect3.size = Vector2(100/2, 100/2);
    rect3.anchors = Vector2(1, 1);
    UIImageComponent& uiImageRenderer3 = scene->AddComponent<UIImageComponent>(uiImage3);
    uiImageRenderer3.sourceImage = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/brickwall.jpg"); 
    uiImageRenderer3.material = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Sprite.glsl"));
    scene->SetParent(uiImage2, uiImage3);

    Entity uiText = scene->AddEntity("UiText");
    RectTransformComponet& rect4 = scene->AddComponent<RectTransformComponet>(uiText);
    rect4.pos = {0, 0};
    rect4.size = Vector2(2, 2);
    rect4.anchors = Vector2(1, 1);
    UITextComponent& uiTextRenderer = scene->AddComponent<UITextComponent>(uiText);
    uiTextRenderer.text = "Lolo";
    uiTextRenderer.color = {0.5f, 0.8f, 0.2f, 1.0f};
    uiTextRenderer.font = Font::CreateFromFile("Engine/Fonts/OpenSans/static/OpenSans_Condensed-Bold.ttf");
    uiTextRenderer.material = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Font.glsl"));
    scene->SetParent(uiImage3, uiText);*/
    

    Entity env = scene->AddEntity("Env");
    scene->AddComponent<EnvironmentComponent>(env).settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};

    Entity light = scene->AddEntity("Light");
    LightComponent& lightComponent = scene->AddComponent<LightComponent>(light);
    lightComponent.color = {1,1,1};
    scene->GetComponent<TransformComponent>(light).Position(Vector3(-2, 4, -1));
    scene->GetComponent<TransformComponent>(light).LocalEulerAngles(Vector3(45, -125, 0));
    lightComponent.renderShadow = false;

    camera = scene->AddEntity("Camera");
    CameraComponent& cam = scene->AddComponent<CameraComponent>(camera);
    scene->GetComponent<TransformComponent>(camera).LocalPosition(Vector3(0, 15, 15));
    scene->GetComponent<TransformComponent>(camera).LocalEulerAngles(Vector3(-25, 0, 0));
    scene->AddComponent<ScriptComponent>(camera).AddScript<CameraMovementScript>()->moveSpeed = 60;
    //camMove.transform = &camera->transform();
    //camMove.moveSpeed = 60;
    //camMove.OnInit();
    cam.farClipPlane = 1000;

    Ref<Model> floorModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/plane.obj");
    Ref<Model> cubeModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/Cube.obj");

    Entity floorEntity = scene->AddEntity("Floor");
    ModelRendererComponent& floorRenderer = scene->AddComponent<ModelRendererComponent>(floorEntity);
    floorRenderer.SetModel(floorModel);
    floorRenderer.GetMaterialsOverride()[0] = LoadFloorMaterial();
    //floorRenderer.GetMaterialsOverride()[1] = LoadFloorMaterial();
    RigidbodyComponent& floorEntityP = scene->AddComponent<RigidbodyComponent>(floorEntity);
    floorEntityP.SetShape(CollisionShape::BoxShape({25,0.1f,25}));
    floorEntityP.Mass(0);
    floorEntityP.SetType(RigidbodyComponent::Type::Static);
    floorEntityP.NeverSleep(true);
    //floorEntityP->entity()->transform().localEulerAngles({0,0,-25});

    Entity character2Entity = scene->AddEntity("MainCube");
    ModelRendererComponent& character2Renderer = scene->AddComponent<ModelRendererComponent>(character2Entity);
    character2Renderer.SetModel(cubeModel);
    character2Renderer.GetMaterialsOverride()[0] = LoadRockMaterial();
    //character2Renderer.GetMaterialsOverride()[1] = LoadRockMaterial();
    RigidbodyComponent& physicObject = scene->AddComponent<RigidbodyComponent>(character2Entity);
    physicObject.SetShape(CollisionShape::BoxShape({1,1,1}));
    physicObject.Mass(1);
    physicObject.NeverSleep(true);
    scene->GetComponent<TransformComponent>(character2Entity).Position({2, 13, 0});
    scene->GetComponent<TransformComponent>(character2Entity).Rotation(QuaternionIdentity);

    /*Entity character2Entity2 = scene->AddEntity("MainCube2");
    ModelRendererComponent& character2Renderer2 = character2Entity2.AddComponent<ModelRendererComponent>();
    character2Renderer2.SetModel(cubeModel);
    character2Renderer2.GetMaterialsOverride()[0] = LoadRockMaterial();
    RigidbodyComponent& physicObject2 = character2Entity2.AddComponent<RigidbodyComponent>();
    physicObject2.SetShape(CollisionShape::BoxShape({1,1,1}));
    physicObject2.Mass(1);
    physicObject2.NeverSleep(true);
    character2Entity2.GetComponent<TransformComponent>().Position({-3, 13, 0});
    character2Entity2.GetComponent<TransformComponent>().Rotation(QuaternionIdentity);
    JointComponent& joint = character2Entity2.AddComponent<JointComponent>();
    joint.pivot = Vector3{-3, 13, 0};
    joint.rb = character2Entity2.Id();*/

    Entity trigger = scene->AddEntity("Trigger");
    RigidbodyComponent& _trigger = scene->AddComponent<RigidbodyComponent>(trigger);
    _trigger.SetShape(CollisionShape::BoxShape({4,1,4}));
    _trigger.SetType(RigidbodyComponent::Type::Trigger);
    _trigger.NeverSleep(true);

    // Fixme: Not Work why play mode clone the scene and theirs system, Work only if Start Scene now
    /*scene->GetSystem<PhysicsSystem>()->AddOnTriggerEnterCallback([](Scene& scene, Entity trigger, Entity other){
        LogWarning("OnTrigger");
        scene.GetComponent<RigidbodyComponent>(other).ApplyImpulse(Vector3Up * 25.0f);
        //scene->GetComponent<RigidbodyComponent>(other).ApplyImpulse(Vector3Up * 25.0f);
    });*/

    Entity luaScript = scene->AddEntity("LuaScript");
    LuaScriptComponent& _luaScript = scene->AddComponent<LuaScriptComponent>(luaScript);
    _luaScript.scriptPath = "Sandbox/LuaScripts/Test.lua";

    Entity luaScript2 = scene->AddEntity("LuaScript2");
    LuaScriptComponent& _luaScript2 = scene->AddComponent<LuaScriptComponent>(luaScript2);
    _luaScript2.scriptPath = "Sandbox/LuaScripts/Test2.lua";

    //scene->Save("res/scene1.scene");
    
    //scene->Start();
    Application::AddModule<Editor>();

    /*typedef Module* (*CreateInstanceFunc)();
    void* module = Platform::LoadDynamicLibrary("build/Release/dynamic_module.dll");
    CreateInstanceFunc func = (CreateInstanceFunc)Platform::LoadDynamicFunction(module, "CreateInstance");
    Application::AddModule(func());*/
}

void PhysicsSample::OnUpdate(float deltaTime){
    //return;
    Scene* scene = SceneManager::Get().GetActiveScene();
    //scene->Update();
    if(scene->Running() == false) return;

    TransformComponent& camT = scene->GetComponent<TransformComponent>(camera);
    RayResult hit;
    //Throwing a Possible Null Expection Pointer Here
    if(scene->GetSystem<PhysicsSystem>()->Raycast(camT.Position(), camT.Back() * 1000.0f, hit)){
        LogInfo("Hitting: %s", scene->GetComponent<InfoComponent>(hit.entity).name.c_str());
    }

    /*Assert(scene->GetRegistry().ctx().get<PhysicsSystem*>() == scene->GetSystem<PhysicsSystem>());
    auto physicsSystem = scene->GetRegistry().ctx().get<PhysicsSystem*>();
    if(physicsSystem->Raycast(camT.Position(), camT.Back() * 1000.0f, hit)){
        LogInfo("Hitting: %s", hit.entity.GetComponent<InfoComponent>().name.c_str());
    }*/

    if(Input::IsKeyDown(KeyCode::R)){
        Entity e = SceneManager::Get().GetActiveScene()->AddEntity("PhysicsCube");
        scene->GetComponent<TransformComponent>(e).Position({2, 13, 0});
        scene->GetComponent<TransformComponent>(e).Rotation(QuaternionIdentity);
        scene->AddComponent<ScriptComponent>(e).AddScript<PhysicsCubeS>()->timeToDestroy = 100000000;
    }

    if(Input::IsKeyDown(KeyCode::T)){
        Entity e = SceneManager::Get().GetActiveScene()->InstantiatePrefab("Sandbox/test.prefab");
        scene->GetComponent<TransformComponent>(e).Position({2, 13, 0});
        scene->GetComponent<TransformComponent>(e).Rotation(QuaternionIdentity);
        scene->AddComponent<ScriptComponent>(e).AddScript<PhysicsCubeS>();
    }
}   

void PhysicsSample::OnRender(float deltaTime){
    //SceneManager::Get().GetActiveScene()->Draw();
    //scene->GetSystem<PhysicsSystem>()->ShowDebugGizmos();
}

void PhysicsSample::OnGUI(){
    /*
    ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    static bool b = true;
    ImGui::ShowDemoWindow(&b);

    //ImGuiWindowFlags window_flags = 0;
    //window_flags |= ImGuiWindowFlags_NoBackground;
    //window_flags |= ImGuiWindowFlags_NoTitleBar;

    static bool b2 = true;
    ImGui::Begin("Entities", &b2);

    auto view = scene->GetRegistry().view<TransformComponent, InfoComponent>();
    for(auto e: view){
        TransformComponent& transform = view.get<TransformComponent>(e);
        InfoComponent& info = view.get<InfoComponent>(e);

        ImGui::Text(info.name.c_str());
    }

    ImGui::End();
    */
}

void PhysicsSample::OnResize(int width, int height){}
void PhysicsSample::OnExit(){}