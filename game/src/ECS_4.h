#pragma once

#include <OD/OD.h>
//#include <OD/AnimationSystem/Animator.h>
#include "CameraMovement.h"
#include <assert.h>
#include "Ultis.h"
#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_speech.h>
#include <soloud_thread.h>
#include <thread>
#include <future>

using namespace OD;

struct RotateScript: public Script{
    float speed = 40;

    void Serialize(ArchiveNode& s) override {
        s.Add(&speed, "speed");
    }

    void OnUpdate() override {
        TransformComponent& transform = entity().GetComponent<TransformComponent>();
        transform.localEulerAngles(Vector3(transform.localEulerAngles().x, Platform::GetTime() * speed, transform.localEulerAngles().z));
    }
};

struct ECS_4: public OD::Module {
    //CameraMovement camMove;

    Entity mainEntity;
    Entity camera;
    Entity light;
    Entity otherEntity;

    Transform gismoTransform;

    std::future<void> playMusic;

    SoLoud::Soloud soloud;
    SoLoud::Wav sample;

    void AddTransparent(Vector3 pos){
        Entity et = SceneManager::Get().activeScene()->AddEntity("Transparent");
        et.GetComponent<TransformComponent>().position(pos);
        et.GetComponent<TransformComponent>().localScale(Vector3(10, 10, 10));
        MeshRendererComponent& _meshRenderer3 = et.AddComponent<MeshRendererComponent>();
        _meshRenderer3.model(AssetManager::Get().LoadModel("res/Builtins/Models/plane.obj"));
        _meshRenderer3.model()->materials[0]->shader(AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/UnlitBlend.glsl"));
        _meshRenderer3.model()->materials[0]->SetTexture("mainTex", AssetManager::Get().LoadTexture2D("res/Builtins/Textures/blending_transparent.png", {TextureFilter::Linear, true}));
    }

    void OnInit() override {
        LogInfo("Game Init");

        soloud.init();

        //LogInfo("Size of: %zd", sizeof(ArchiveNode));
    
        /*sample.load("res/sounds/2ne1_2.mp3");
        soloud.play(sample);*/

        Application::vsync(false);

        SceneManager::Get().RegisterSystem<PhysicsSystem>("PhysicsSystem");
        SceneManager::Get().RegisterSystem<StandRendererSystem>("StandRendererSystem");
        SceneManager::Get().RegisterSystem<ScriptSystem>("ScriptSystem");
        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");
        SceneManager::Get().RegisterScript<RotateScript>("RotateScript");

        OD::Scene* scene = SceneManager::Get().NewScene();
        //scene->GetSystem<StandRendererSystem>()->sceneLightSettings.ambient = Vector3(0.11f,0.16f,0.25f) * 1.0f;
        //scene->Start();

        Ref<Model> floorModel = AssetManager::Get().LoadModel(
            "res/models/plane.glb",
            AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/StandDiffuse.glsl")
        );
        //floorModel->materials[0]->shader = AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/StandDiffuse.glsl");
        //floorModel->materials[0]->SetTexture("mainTex", AssetManager::Get().LoadTexture2D("res/textures/floor.jpg", OD::TextureFilter::Linear, false));
        //floorModel->materials[0]->SetVector4("color", Vector4(1, 1, 1, 1));

        Ref<Model> cubeModel = AssetManager::Get().LoadModel(
            "res/models/Cube.glb",
            AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/StandDiffuse.glsl")
        );
        //cubeModel->materials[0]->shader = AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/StandDiffuse.glsl");
        //cubeModel->materials[0]->SetTexture("mainTex", AssetManager::Get().LoadTexture2D("res/textures/floor.jpg", OD::TextureFilter::Linear, false));
        //cubeModel->materials[0]->SetVector4("color", Vector4(1, 1, 1, 1));

        Entity env = scene->AddEntity("Env");
        env.AddComponent<EnvironmentComponent>().settings.ambient = Vector3(0.11f,0.16f,0.25f);

        Entity e = scene->AddEntity("Floor");
        e.GetComponent<TransformComponent>().position(Vector3(0,-2, 0));
        e.GetComponent<TransformComponent>().localScale(Vector3(10, 1, 10));
        MeshRendererComponent& _meshRenderer = e.AddComponent<MeshRendererComponent>();
        _meshRenderer.model(floorModel);
        _meshRenderer.materialsOverride()[0] = AssetManager::Get().LoadMaterial(std::string("res/textures/floor.material"));
        LogInfo("LoadMaterial: %s Hash: %zd", std::string("res/textures/floor.material").c_str(), std::hash<std::string>{}(std::string("res/textures/floor.material")));

        Entity e2 = scene->AddEntity("Cube");
        e2.GetComponent<TransformComponent>().position(Vector3(-8, 0, -4));
        e2.GetComponent<TransformComponent>().localScale(Vector3(4*1, 4*1, 4*1));
        MeshRendererComponent& _meshRenderer2 = e2.AddComponent<MeshRendererComponent>();
        _meshRenderer2.model(cubeModel);
        _meshRenderer2.materialsOverride()[0] = AssetManager::Get().LoadMaterial("res/textures/floor.material");

        /*Entity sponza = scene->AddEntity("Sponza");
        sponza.GetComponent<TransformComponent>().position(Vector3(0,0,0));
        sponza.GetComponent<TransformComponent>().localScale(Vector3(1, 1, 1));
        MeshRendererComponent& sponzaModel = sponza.AddComponent<MeshRendererComponent>();
        sponzaModel.model(AssetManager::Get().LoadModel(
            "res/Sponza/sponza.obj",
            AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/StandDiffuse.glsl")
        ));*/

        AddTransparent(Vector3(5, 3, -8));
        AddTransparent(Vector3(6, 3, -12));
        AddTransparent(Vector3(8, 3, -20));

        Entity camera = scene->AddEntity("Camera");
        CameraComponent& cam = camera.AddComponent<CameraComponent>();
        camera.GetComponent<TransformComponent>().localPosition(Vector3(0, 2, 4));
        camera.GetComponent<TransformComponent>().localEulerAngles(Vector3(-25, 0, 0));
        camera.AddComponent<ScriptComponent>().AddScript<CameraMovementScript>()->moveSpeed = 60;
        //camMove.transform = &camera->GetComponent<TransformComponent>()();
        //camMove.moveSpeed = 60;
        cam.farClipPlane = 1000;

        mainEntity = scene->AddEntity("Main");
        mainEntity.GetComponent<TransformComponent>().localPosition(Vector3(2, 0, 0));
        /*MeshRendererComponent& meshRenderer = mainEntity.AddComponent<MeshRendererComponent>();
        meshRenderer.model(AssetManager::Get().LoadModel(
            "res/models/backpack/backpack.obj",
            AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/StandDiffuse.glsl")
        ));
        meshRenderer.subMeshIndex(-1);*/
        mainEntity.AddComponent<ScriptComponent>().AddScript<RotateScript>();

        light = scene->AddEntity("Directional Light");
        LightComponent& lightComponent = light.AddComponent<LightComponent>();
        lightComponent.color = Vector3(1,1,1);
        lightComponent.intensity = 1;
        lightComponent.renderShadow = false;
        light.GetComponent<TransformComponent>().position(Vector3(-2, 4, -1));
        light.GetComponent<TransformComponent>().localEulerAngles(Vector3(45, -125, 0));
        
        ///*
        Entity pointLight = scene->AddEntity("Point Light");
        LightComponent& _pointLight = pointLight.AddComponent<LightComponent>();
        _pointLight.type = LightComponent::Type::Point;
        _pointLight.color = Vector3(1,1,1);
        _pointLight.intensity = 5;
        _pointLight.radius = 10;
        pointLight.GetComponent<TransformComponent>().position(Vector3(4, 4, 0));

        Entity pointLight2 = scene->AddEntity("Point Light 2");
        LightComponent& _pointLight2 = pointLight2.AddComponent<LightComponent>();
        _pointLight2.type = LightComponent::Type::Point;
        _pointLight2.color = Vector3(0,0,1);
        _pointLight2.intensity = 5;
        _pointLight2.radius = 20;
        pointLight2.GetComponent<TransformComponent>().position(Vector3(-3, 0.5f, 0));
        //*/

        for(int i = 0; i < 10000; i++){
            float posRange = 200;

            Entity e = scene->AddEntity("Entity" + std::to_string(random(0, 200)));
            e.AddComponent<ScriptComponent>().AddScript<RotateScript>();
            MeshRendererComponent& mr = e.AddComponent<MeshRendererComponent>();
            mr.model(cubeModel);
            mr.materialsOverride()[0] = AssetManager::Get().LoadMaterial("res/textures/floor.material");
        
            float angle = 20.0f * i; 
            e.GetComponent<TransformComponent>().localPosition(Vector3(random(-posRange, posRange), random(0, posRange), random(-posRange, posRange)));
            e.GetComponent<TransformComponent>().localEulerAngles(Vector3(random(-180, 180), random(-180, 180), random(-180, 180)));
            otherEntity = e;

            scene->SetParent(mainEntity.id(), e.id());
        }

        Application::AddModule<Editor>();
        //scene->Start();
    }

    void OnUpdate(float deltaTime) override {
        //camMove.OnUpdate();

        //mainEntity.GetComponent<TransformComponent>().localEulerAngles(Vector3(0, Platform::GetTime() * 40, 0));
        //otherEntity.GetComponent<TransformComponent>().position(Vector3(-5, 2, -1.5f));

        SceneManager::Get().activeScene()->Update();

        if(Input::IsKeyDown(KeyCode::A)){
            LogInfo("OnKeyDown A");
        }

        if(Input::IsKeyUp(KeyCode::A)){
            LogInfo("OnKeyUp A");
        }

        if(Input::IsMouseButtonDown(MouseButton::Left)){
            LogInfo("OMouseButtonDown Left");
        }
    }   

    void OnRender(float deltaTime) override {
        //Renderer::SetDepthTest(DepthTest::LESS);
        //Renderer::SetCullFace(CullFace::BACK);

        SceneManager::Get().activeScene()->Draw();

        /*Vector3 pos = otherEntity.GetComponent<TransformComponent>().position();

        gismoTransform.localRotation(otherEntity.GetComponent<TransformComponent>().rotation());

        gismoTransform.localPosition(pos);
        gismoTransform.localScale(Vector3(1.1f, 1.1f, 1.1f));
        Renderer::DrawWireCube(gismoTransform.GetLocalModelMatrix(), Vector3(0,0,1), 1);

        
        gismoTransform.localPosition(light.GetComponent<TransformComponent>().position());
        gismoTransform.localScale(Vector3(0.2f, 0.2f, 0.2f));
        Renderer::DrawWireCube(gismoTransform.GetLocalModelMatrix(), Vector3(0,0,1), 1);
        Renderer::DrawLine(
            light.GetComponent<TransformComponent>().position(), 
            light.GetComponent<TransformComponent>().position() + light.GetComponent<TransformComponent>().forward(),
            Vector3(1,0,1), 
            2
        );*/
    }

    void OnGUI() override {
        /*
        ImGui::Begin("Directional Light");

        static float translation[] = {45, -125, 0.0};
        ImGui::SliderFloat3("light rotation", translation, -180, 180);
        light.GetComponent<TransformComponent>().localEulerAngles(Vector3(translation[0], translation[1], translation[2]));

        static float color[4] = { 1.0f,1.0f,1.0f,1.0f };
        ImGui::ColorEdit3("color", color);
        light.GetComponent<LightComponent>().color = Vector3(color[0], color[1], color[2]);
        
        ImGui::End();
        */
    }

    void OnResize(int width, int height) override{
    }

};