#pragma once

#include <OD/OD.h>
#include "Ultis/CameraMovement.h"
#include <assert.h>
#include "Ultis/Ultis.h"
#include <entt/entt.hpp>
#include <sol/sol.hpp>
#include <OD/LuaScripting/LuaScripts.h>

using namespace OD;

struct PhysicsCubeS: public Script{
    float t;
    float timeToDestroy = 5;

    void OnStart() override{
        sol::state lua;
        int x = 0;
        lua.set_function("beep", [&x]{ ++x; });
        lua.script("beep()");
        Assert(x == 1);

        LogInfo("PhysicsCubeS OnStart");
        
        Assert(GetEntity().IsValid() == true);

        Entity& entity = GetEntity();

        Ref<Model> cubeModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/Cube.glb");
        //cubeModel->SetShader(AssetManager::GetGlobal()->LoadShaderFromFile("res/Builtins/Shaders/Unlit.glsl"));
        //cubeModel->materials[0].SetTexture("mainTex", AssetManager::GetGlobal()->LoadTexture2D("res/textures/rock.jpg", false, OD::TextureFilter::Linear, false));
        //cubeModel->materials[0].SetVector4("color", Vector4(1, 1, 1, 1));
        cubeModel->materials[0] = LoadMaterial1();

        ModelRendererComponent& renderer = entity.AddOrGetComponent<ModelRendererComponent>();
        renderer.SetModel(cubeModel);

        RigidbodyComponent& physicObject = entity.AddOrGetComponent<RigidbodyComponent>();
        physicObject.NeverSleep(true);
        
        //physicObject->boxShapeSize = {1,1,1};
        //physicObject->mass = 1;

        physicObject.SetShape(CollisionShape::BoxShape({1,1,1}));
        //physicObject->SetMass(1);
    }

    void OnUpdate() override{
        ///*
        t += Application::DeltaTime();
        if(t > timeToDestroy){
            GetEntity().GetScene()->DestroyEntity(GetEntity().Id());
            //LogInfo("ToDestroy");
        }
        //*/
    }

    void OnDestroy() override{
        LogInfo("PhysicsCubeS OnDestroy");
    }

    template <class Archive>
    void serialize(Archive & ar){}
};

struct PhysicsSample: OD::Module {
    //CameraMovement camMove;
    Entity camera;

    void OnInit() override {
        LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

        SceneManager::Get().RegisterScript<PhysicsCubeS>("PhysicsCubeS");
        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

        Scene* scene = SceneManager::Get().NewScene();

        Entity text = scene->AddEntity("Text");
        text.GetComponent<TransformComponent>().LocalPosition(Vector3(25.0f, 25.0f, 0));
        TextRendererComponent& textRenderer = text.AddComponent<TextRendererComponent>();
        textRenderer.text = "Ai meu cu!!!";
        textRenderer.color = {0.5f, 0.8f, 0.2f, 1.0f};
        textRenderer.font = CreateRef<Font>("Engine/Fonts/OpenSans/static/OpenSans_Condensed-Bold.ttf");
        textRenderer.material = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Font.glsl"));

        Entity env = scene->AddEntity("Env");
        env.AddComponent<EnvironmentComponent>().settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};

        Entity light = scene->AddEntity("Light");
        LightComponent& lightComponent = light.AddComponent<LightComponent>();
        lightComponent.color = {1,1,1};
        light.GetComponent<TransformComponent>().Position(Vector3(-2, 4, -1));
        light.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(45, -125, 0));
        lightComponent.renderShadow = false;

        camera = scene->AddEntity("Camera");
        CameraComponent& cam = camera.AddComponent<CameraComponent>();
        camera.GetComponent<TransformComponent>().LocalPosition(Vector3(0, 15, 15));
        camera.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(-25, 0, 0));
        camera.AddComponent<ScriptComponent>().AddScript<CameraMovementScript>()->moveSpeed = 60;
        //camMove.transform = &camera->transform();
        //camMove.moveSpeed = 60;
        //camMove.OnInit();
        cam.farClipPlane = 1000;

        Ref<Model> floorModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/plane.glb");
        Ref<Model> cubeModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/Cube.glb");

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

        Entity character2Entity = scene->AddEntity("MainCube");
        ModelRendererComponent& character2Renderer = character2Entity.AddComponent<ModelRendererComponent>();
        character2Renderer.SetModel(cubeModel);
        character2Renderer.GetMaterialsOverride()[0] = LoadRockMaterial();
        RigidbodyComponent& physicObject = character2Entity.AddComponent<RigidbodyComponent>();
        physicObject.SetShape(CollisionShape::BoxShape({1,1,1}));
        physicObject.Mass(1);
        physicObject.NeverSleep(true);
        character2Entity.GetComponent<TransformComponent>().Position({2, 13, 0});
        character2Entity.GetComponent<TransformComponent>().Rotation(QuaternionIdentity);

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
        RigidbodyComponent& _trigger = trigger.AddComponent<RigidbodyComponent>();
        _trigger.SetShape(CollisionShape::BoxShape({4,1,4}));
        _trigger.SetType(RigidbodyComponent::Type::Trigger);
        _trigger.NeverSleep(true);

        scene->GetSystem<PhysicsSystem>()->AddOnTriggerEnterCallback([](Entity trigger, Entity other){
            LogInfo("OnTrigger");
            other.GetComponent<RigidbodyComponent>().ApplyImpulse(Vector3Up * 25.0f);
        });

        Entity luaScript = scene->AddEntity("LuaScript");
        LuaScriptComponent& _luaScript = luaScript.AddComponent<LuaScriptComponent>();
        _luaScript.scriptPath = "Sandbox/LuaScripts/Test.lua";

        Entity luaScript2 = scene->AddEntity("LuaScript2");
        LuaScriptComponent& _luaScript2 = luaScript2.AddComponent<LuaScriptComponent>();
        _luaScript2.scriptPath = "Sandbox/LuaScripts/Test2.lua";

        //scene->Save("res/scene1.scene");
        //scene->Start();
        Application::AddModule<Editor>();

        /*typedef Module* (*CreateInstanceFunc)();
        void* module = Platform::LoadDynamicLibrary("build/Release/dynamic_module.dll");
        CreateInstanceFunc func = (CreateInstanceFunc)Platform::LoadDynamicFunction(module, "CreateInstance");
        Application::AddModule(func());*/
    }

    void OnUpdate(float deltaTime) override {
        Scene* scene = SceneManager::Get().GetActiveScene();
        //scene->Update();
        if(scene->Running() == false) return;

        TransformComponent& camT = camera.GetComponent<TransformComponent>();
        RayResult hit;
        //Throwing a Possible Null Expection Pointer Here
        if(scene->GetSystem<PhysicsSystem>()->Raycast(camT.Position(), camT.Back() * 1000.0f, hit)){
            LogInfo("Hitting: %s", hit.entity.GetComponent<InfoComponent>().name.c_str());
        }

        /*Assert(scene->GetRegistry().ctx().get<PhysicsSystem*>() == scene->GetSystem<PhysicsSystem>());
        auto physicsSystem = scene->GetRegistry().ctx().get<PhysicsSystem*>();
        if(physicsSystem->Raycast(camT.Position(), camT.Back() * 1000.0f, hit)){
            LogInfo("Hitting: %s", hit.entity.GetComponent<InfoComponent>().name.c_str());
        }*/

        if(Input::IsKeyDown(KeyCode::R)){
            Entity e = SceneManager::Get().GetActiveScene()->AddEntity("PhysicsCube");
            e.GetComponent<TransformComponent>().Position({2, 13, 0});
            e.GetComponent<TransformComponent>().Rotation(QuaternionIdentity);
            e.AddComponent<ScriptComponent>().AddScript<PhysicsCubeS>();
        }

        if(Input::IsKeyDown(KeyCode::T)){
            Entity e = SceneManager::Get().GetActiveScene()->InstantiatePrefab("Sandbox/test.prefab");
            e.GetComponent<TransformComponent>().Position({2, 13, 0});
            e.GetComponent<TransformComponent>().Rotation(QuaternionIdentity);
            e.AddComponent<ScriptComponent>().AddScript<PhysicsCubeS>();
        }
    }   

    void OnRender(float deltaTime) override {
        //SceneManager::Get().GetActiveScene()->Draw();
        //scene->GetSystem<PhysicsSystem>()->ShowDebugGizmos();
    }

    void OnGUI() override {
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

    void OnResize(int width, int height) override {}
    void OnExit() override {}
};