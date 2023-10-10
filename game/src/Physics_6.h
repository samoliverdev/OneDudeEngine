#pragma once

#include <OD/OD.h>
#include "CameraMovement.h"
#include <assert.h>
#include "Ultis.h"
#include <entt/entt.hpp>

using namespace OD;

struct Test1{
    float a;

    void Serialize(Archive& ar){
        ar.name("Test1");

        ar.Add(&a, "a");
    }
};

struct Test2{
    std::vector<Test1> b;

    void Serialize(Archive& ar){
        ar.name("Test2");

        b.push_back({20});
        b.push_back({254});

        ar.Add(b, "b");
    }
};

struct TestC{
    std::string id = "TestId";
    float speed = 20;
    std::vector<Test1> test1s = {{20}, {245}};

    Test1 test1 = {55};

    void Serialize(Archive& s){
        s.name("TestC");

        s.Add(&id, "id");
        s.Add(&speed, "speed");
        s.Add(test1s, "test1s");
        s.Add(test1, "test1");

        //LogInfo("OnSerialize TestC %zd", s.Values().size());
    }
};

struct PhysicsCubeS: public Script{
    float t;

    void Serialize(Archive& s) override{
        s.name("PhysicsCubeS");
        s.Add(&t, "t");
    }

    void OnStart() override{
        LogInfo("PhysicsCubeS OnStart");
        
        Assert(entity().IsValid() == true);

        Ref<Model> cubeModel = AssetManager::Get().LoadModel("res/models/Cube.glb");
        //cubeModel->SetShader(AssetManager::GetGlobal()->LoadShaderFromFile("res/Builtins/Shaders/Unlit.glsl"));
        //cubeModel->materials[0].SetTexture("mainTex", AssetManager::GetGlobal()->LoadTexture2D("res/textures/rock.jpg", false, OD::TextureFilter::Linear, false));
        //cubeModel->materials[0].SetVector4("color", Vector4(1, 1, 1, 1));
        cubeModel->materials[0] = AssetManager::Get().LoadMaterial("res/mat1.material"); //Material::CreateFromFile("res/mat1.material");

        entity().GetComponent<TransformComponent>().position({2, 13, 0});
        entity().GetComponent<TransformComponent>().rotation(Quaternion::identity);

        MeshRendererComponent& renderer = entity().AddOrGetComponent<MeshRendererComponent>();
        renderer.model(cubeModel);

        RigidbodyComponent& physicObject = entity().AddOrGetComponent<RigidbodyComponent>();
        
        //physicObject->boxShapeSize = {1,1,1};
        //physicObject->mass = 1;

        physicObject.shape({1,1,1});
        //physicObject->SetMass(1);
    }

    void OnUpdate() override{
        ///*
        t += Application::deltaTime();
        if(t > 5){
            entity().scene()->DestroyEntity(entity().id());
            //LogInfo("ToDestroy");
        }
        //*/
    }

    void OnDestroy() override{
        LogInfo("PhysicsCubeS OnDestroy");
    }
};

struct Physics_6: OD::Module {
    Scene* scene;
    //CameraMovement camMove;
    Entity camera;

    bool spawnInput;
    bool lastSpawnInput;

    void OnInit() override {
        LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

        using namespace entt::literals;

        entt::meta<TestC>()
            .type(entt::type_hash<TestC>::value())
            .data<&TestC::id>("id"_hs)
            .data<&TestC::speed>("speed"_hs)
            .data<&TestC::test1s>("test1s"_hs)
            .data<&TestC::test1>("test1"_hs);

        TestC t;
        t.id = "lolo";

        for(auto &&[id, type]: entt::resolve<TestC>().base()) {
            std::cout << type.info().name() << std::endl;
        }

        auto _id = entt::resolve<TestC>().data("id"_hs);
        LogInfo("Test: %s", _id.get(t).cast<std::string>().c_str());

        /*Test2 t2;
        Archive a;
        t2.Serialize(a);
        a.Show();*/
        
        SceneManager::Get().RegisterComponent<TestC>("TestC");
        SceneManager::Get().RegisterScript<PhysicsCubeS>("PhysicsCubeS");
        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

        SceneManager::Get().RegisterSystem<PhysicsSystem>("PhysicsSystem");
        SceneManager::Get().RegisterSystem<StandRendererSystem>("StandRendererSystem");
        SceneManager::Get().RegisterSystem<ScriptSystem>("ScriptSystem");

        scene = SceneManager::Get().NewScene();

        //scene->AddSystem<PhysicsSystem>();
        //scene->AddSystem<StandRendererSystem>();
        //scene->AddSystem<ScriptSystem>();

        //scene.Load("res/scene1.scene");
        //return true;

        //Application::SetVSync(false);

        //scene->GetSystem<StandRendererSystem>()->sceneLightSettings.ambient = Vector3(0.11f,0.16f,0.25f) * 1.0f;

        Entity env = scene->AddEntity("Env");
        env.AddComponent<EnvironmentComponent>().settings.ambient = Vector3(0.11f,0.16f,0.25f);

        Entity light = scene->AddEntity("Light");
        LightComponent& lightComponent = light.AddComponent<LightComponent>();
        lightComponent.color = Vector3(1,1,1);
        light.GetComponent<TransformComponent>().position(Vector3(-2, 4, -1));
        light.GetComponent<TransformComponent>().localEulerAngles(Vector3(45, -125, 0));

        camera = scene->AddEntity("Camera");
        CameraComponent& cam = camera.AddComponent<CameraComponent>();
        camera.GetComponent<TransformComponent>().localPosition(Vector3(0, 15, 15));
        camera.GetComponent<TransformComponent>().localEulerAngles(Vector3(-25, 0, 0));
        camera.AddComponent<TestC>();
        camera.AddComponent<ScriptComponent>().AddScript<CameraMovementScript>()->moveSpeed = 60;
        //camMove.transform = &camera->transform();
        //camMove.moveSpeed = 60;
        //camMove.OnInit();
        cam.farClipPlane = 1000;

        Ref<Model> floorModel = AssetManager::Get().LoadModel("res/models/plane.glb");
        //floorModel->materials[0] = AssetManager::Get().LoadMaterial("res/textures/floor.material");
        //floorModel->materials[0]->shader = AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/StandDiffuse.glsl");
        //floorModel->materials[0]->shader = AssetManager::GetGlobal()->LoadShaderFromFile("res/Builtins/Shaders/Unlit.glsl");
        //floorModel->materials[0]->SetTexture("mainTex", AssetManager::GetGlobal()->LoadTexture2D("res/textures/floor.jpg", false, OD::TextureFilter::Linear, false));
        //floorModel->materials[0]->SetVector4("color", Vector4(1, 1, 1, 1));

        //floorModel->materials[0]->Save(std::string("res/mat1.material"));

        Ref<Model> cubeModel = Model::CreateFromFile("res/models/Cube.glb");
        //cubeModel->materials[0] = AssetManager::Get().LoadMaterial("res/textures/rock.material");
        //cubeModel->materials[0]->shader = AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/StandDiffuse.glsl");
        //cubeModel->materials[0]->shader = AssetManager::GetGlobal()->LoadShaderFromFile("res/Builtins/Shaders/Unlit.glsl");
        //cubeModel->materials[0]->SetTexture("mainTex", AssetManager::GetGlobal()->LoadTexture2D("res/textures/rock.jpg", false, OD::TextureFilter::Linear, false));
        //cubeModel->materials[0]->SetVector4("color", Vector4(1, 1, 1, 1));

        Entity floorEntity = scene->AddEntity("Floor");
        MeshRendererComponent& floorRenderer = floorEntity.AddComponent<MeshRendererComponent>();
        floorRenderer.model(floorModel);
        floorRenderer.materialsOverride()[0] = AssetManager::Get().LoadMaterial("res/textures/floor.material");
        RigidbodyComponent& floorEntityP = floorEntity.AddComponent<RigidbodyComponent>();
        floorEntityP.shape({25,0.1f,25});
        floorEntityP.mass(0);
        floorEntityP.type(RigidbodyComponent::Type::Static);
        //floorEntityP.neverSleep(true);
        //floorEntityP->entity()->transform().localEulerAngles({0,0,-25});

        Entity character2Entity = scene->AddEntity("MainCube");
        //scene->SetParent(floorEntity.id(), character2Entity.id());

        MeshRendererComponent& character2Renderer = character2Entity.AddComponent<MeshRendererComponent>();
        character2Renderer.model(cubeModel);
        character2Renderer.materialsOverride()[0] = AssetManager::Get().LoadMaterial("res/textures/rock.material");
        RigidbodyComponent& physicObject = character2Entity.AddComponent<RigidbodyComponent>();
        physicObject.shape({1,1,1});
        physicObject.mass(1);
        //physicObject.neverSleep(true);
        character2Entity.GetComponent<TransformComponent>().position({2, 13, 0});
        character2Entity.GetComponent<TransformComponent>().rotation(Quaternion::identity);

        Entity trigger = scene->AddEntity("Trigger");
        RigidbodyComponent& _trigger = trigger.AddComponent<RigidbodyComponent>();
        _trigger.shape({4,1,4});
        _trigger.type(RigidbodyComponent::Type::Trigger);
        _trigger.neverSleep(true);

        //scene->Save("res/scene1.scene");
        //scene->Start();

        Application::AddModule<Editor>();
    }

    void OnUpdate(float deltaTime) override {
        scene->Update();
        if(scene->running() == false) return;

        /*
        TransformComponent& camT = camera.GetComponent<TransformComponent>();
        RayResult hit;
        if(scene->GetSystem<PhysicsSystem>()->Raycast(camT.position(), camT.back() * 1000.0f, hit)){
            LogInfo("Hitting: %s", hit.entity.GetComponent<InfoComponent>().name.c_str());
        }*/

        lastSpawnInput = spawnInput;
        spawnInput = Input::IsKey(KeyCode::R);

        if(spawnInput == true && lastSpawnInput == false){
            scene->AddEntity("PhysicsCube").AddComponent<ScriptComponent>().AddScript<PhysicsCubeS>();
            scene->Save("res/scene1.scene");
            
            //scene = SceneManager::Get().NewScene();
            //scene->Load("res/scene1.scene");
            //scene->Start();
        }
    }   

    void OnRender(float deltaTime) override {
        scene->Draw();
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
};