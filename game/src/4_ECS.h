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
#include <fstream>
#include <OD/RenderPipeline/StandRenderPipeline2.h>

using namespace OD;

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
        Entity et = SceneManager::Get().GetActiveScene()->AddEntity("Transparent");
        et.GetComponent<TransformComponent>().Position(pos);
        et.GetComponent<TransformComponent>().LocalScale(Vector3(10, 10, 10));
        MeshRendererComponent& _meshRenderer3 = et.AddComponent<MeshRendererComponent>();
        _meshRenderer3.SetModel(AssetManager::Get().LoadModel("res/Engine/Models/plane.obj"));

        for(auto i: _meshRenderer3.GetModel()->materials){
            i->SetShader(AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/UnlitBlend.glsl"));
            i->SetTexture("mainTex", AssetManager::Get().LoadTexture2D("res/Engine/Textures/blending_transparent.png", {TextureFilter::Linear, true}));
        }
    }

    void OnInit() override {
        LogInfo("Game Init");

        soloud.init();

        //LogInfo("Size of: %zd", sizeof(ArchiveNode));
    
        /*sample.load("res/sounds/2ne1_2.mp3");
        soloud.play(sample);*/

        Application::Vsync(false);

        //SceneManager::Get().RegisterSystem<PhysicsSystem>("PhysicsSystem");
        //SceneManager::Get().RegisterSystem<StandRendererSystem>("StandRendererSystem");
        //SceneManager::Get().RegisterSystem<ScriptSystem>("ScriptSystem");
        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");
        SceneManager::Get().RegisterScript<RotateScript>("RotateScript");

        OD::Scene* scene = SceneManager::Get().NewScene();
        //scene->RemoveSystem<StandRenderPipeline>();
        //scene->AddSystem<StandRenderPipeline2>();

        //scene->GetSystem<StandRendererSystem>()->sceneLightSettings.ambient = Vector3(0.11f,0.16f,0.25f) * 1.0f;
        //scene->Start();

        Ref<Model> floorModel = AssetManager::Get().LoadModel(
            "res/Game/Models/plane.glb",
            AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/StandDiffuse.glsl")
        );
        //floorModel->materials[0]->shader = AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/StandDiffuse.glsl");
        //floorModel->materials[0]->SetTexture("mainTex", AssetManager::Get().LoadTexture2D("res/textures/floor.jpg", OD::TextureFilter::Linear, false));
        //floorModel->materials[0]->SetVector4("color", Vector4(1, 1, 1, 1));

        Ref<Model> cubeModel = AssetManager::Get().LoadModel(
            "res/Game/models/Cube.glb",
            AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/StandDiffuse.glsl")
        );
        //cubeModel->materials[0]->shader = AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/StandDiffuse.glsl");
        //cubeModel->materials[0]->SetTexture("mainTex", AssetManager::Get().LoadTexture2D("res/textures/floor.jpg", OD::TextureFilter::Linear, false));
        //cubeModel->materials[0]->SetVector4("color", Vector4(1, 1, 1, 1));

        Entity env = scene->AddEntity("Env");
        env.AddComponent<EnvironmentComponent>().settings.ambient = Vector3(0.11f,0.16f,0.25f);

        Entity e = scene->AddEntity("Floor");
        e.GetComponent<TransformComponent>().Position(Vector3(0,-2, 0));
        e.GetComponent<TransformComponent>().LocalScale(Vector3(10, 1, 10));
        MeshRendererComponent& _meshRenderer = e.AddComponent<MeshRendererComponent>();
        _meshRenderer.SetModel(floorModel);
        _meshRenderer.GetMaterialsOverride()[0] = LoadFloorMaterial();
   
        Entity e2 = scene->AddEntity("Cube");
        e2.GetComponent<TransformComponent>().Position(Vector3(-8, 0, -4));
        e2.GetComponent<TransformComponent>().LocalScale(Vector3(4*1, 4*1, 4*1));
        MeshRendererComponent& _meshRenderer2 = e2.AddComponent<MeshRendererComponent>();
        _meshRenderer2.SetModel(cubeModel);
        _meshRenderer2.GetMaterialsOverride()[0] = LoadFloorMaterial();

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
        camera.GetComponent<TransformComponent>().LocalPosition(Vector3(0, 2, 4));
        camera.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(-25, 0, 0));
        camera.AddComponent<ScriptComponent>().AddScript<CameraMovementScript>()->moveSpeed = 60;
        //camMove.transform = &camera->GetComponent<TransformComponent>()();
        //camMove.moveSpeed = 60;
        cam.farClipPlane = 1000;

        mainEntity = scene->AddEntity("Main");
        mainEntity.GetComponent<TransformComponent>().LocalPosition(Vector3(2, 0, 0));
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
        light.GetComponent<TransformComponent>().Position(Vector3(-2, 4, -1));
        light.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(45, -125, 0));
        
        ///*
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
        //*/

        for(int i = 0; i < 10000; i++){
            float posRange = 200;

            Entity e = scene->AddEntity("Entity" + std::to_string(random(0, 200)));
            e.AddComponent<ScriptComponent>().AddScript<RotateScript>();
            MeshRendererComponent& mr = e.AddComponent<MeshRendererComponent>();
            mr.SetModel(cubeModel);
            mr.GetMaterialsOverride()[0] = LoadFloorMaterial();
        
            float angle = 20.0f * i; 
            e.GetComponent<TransformComponent>().LocalPosition(Vector3(random(-posRange, posRange), random(0, posRange), random(-posRange, posRange)));
            e.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(random(-180, 180), random(-180, 180), random(-180, 180)));
            otherEntity = e;

            scene->SetParent(mainEntity.Id(), e.Id());
        }



        Application::AddModule<Editor>();
        //scene->Start();
    }

    void OnUpdate(float deltaTime) override {
        //camMove.OnUpdate();

        //mainEntity.GetComponent<TransformComponent>().localEulerAngles(Vector3(0, Platform::GetTime() * 40, 0));
        //otherEntity.GetComponent<TransformComponent>().position(Vector3(-5, 2, -1.5f));

        SceneManager::Get().GetActiveScene()->Update();

        /*if(Input::IsKeyDown(KeyCode::A)){
            LogInfo("OnKeyDown A");
        }

        if(Input::IsKeyUp(KeyCode::A)){
            LogInfo("OnKeyUp A");
        }

        if(Input::IsMouseButtonDown(MouseButton::Left)){
            LogInfo("OMouseButtonDown Left");
        }*/
    }   

    void OnRender(float deltaTime) override {
        //Renderer::SetDepthTest(DepthTest::LESS);
        //Renderer::SetCullFace(CullFace::BACK);

        SceneManager::Get().GetActiveScene()->Draw();

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