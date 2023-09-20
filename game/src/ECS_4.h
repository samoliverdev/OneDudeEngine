#pragma once

#include <OD/OD.h>
#include <OD/AnimationSystem/Animator.h>
#include "CameraMovement.h"
#include <assert.h>
#include "Ultis.h"

using namespace OD;

struct RotateScript: public Script{
    float speed = 40;

    void Serialize(Archive& s) override {
        s.name("RotateScript");
        s.Add(&speed, "speed");
    }

    void OnUpdate() override {
        TransformComponent& transform = entity().GetComponent<TransformComponent>();
        transform.localEulerAngles(Vector3(0, Platform::GetTime() * speed, 0));
    }
};

struct ECS_4: public OD::Module {
    OD::Scene* scene;
    //CameraMovement camMove;

    Entity mainEntity;
    Entity camera;
    Entity light;
    Entity otherEntity;

    Transform gismoTransform;

    void AddCharacter(Vector3 pos){
        Entity character = scene->AddEntity("Character");
        character.GetComponent<TransformComponent>().localPosition(pos);
        character.GetComponent<TransformComponent>().localScale(Vector3(10, 10, 10));
        MeshRendererComponent& charMesh = character.AddComponent<MeshRendererComponent>();
        charMesh.model(AssetManager::Get().LoadModel(
            "res/models/Skinned/VampireALusth.dae",
            AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/SkinnedModel.glsl")
        ));
        Ref<Model> anim = AssetManager::Get().LoadModel("res/models/Skinned/Capoeira.dae");
        AnimatorComponent& charAnimator = character.AddComponent<AnimatorComponent>();
        charAnimator.Play(anim->animations[0].get());
    }

    void OnInit() override {
        LogInfo("Game Init");

        Application::vsync(false);

        SceneManager::Get().RegisterSystem<PhysicsSystem>("PhysicsSystem");
        SceneManager::Get().RegisterSystem<StandRendererSystem>("StandRendererSystem");
        SceneManager::Get().RegisterSystem<ScriptSystem>("ScriptSystem");
        SceneManager::Get().RegisterSystem<AnimatorSystem>("AnimatorSystem");
        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");
        SceneManager::Get().RegisterScript<RotateScript>("RotateScript");

        scene = SceneManager::Get().NewScene();
        scene->GetSystem<StandRendererSystem>()->sceneLightSettings.ambient = Vector3(0.11f,0.16f,0.25f) * 1.0f;
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

        Entity e = scene->AddEntity("Floor");
        e.GetComponent<TransformComponent>().position(Vector3(0,-2, 0));
        e.GetComponent<TransformComponent>().localScale(Vector3(10, 1, 10));
        MeshRendererComponent& _meshRenderer = e.AddComponent<MeshRendererComponent>();
        _meshRenderer.model(floorModel);
        _meshRenderer.materialsOverride()[0] = AssetManager::Get().LoadMaterial("res/textures/floor.material");

        Entity e2 = scene->AddEntity("Cube");
        e2.GetComponent<TransformComponent>().position(Vector3(-8, 0, -4));
        e2.GetComponent<TransformComponent>().localScale(Vector3(4*1, 4*1, 4*1));
        MeshRendererComponent& _meshRenderer2 = e2.AddComponent<MeshRendererComponent>();
        _meshRenderer2.model(cubeModel);
        _meshRenderer2.materialsOverride()[0] = AssetManager::Get().LoadMaterial("res/textures/floor.material");

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
        MeshRendererComponent& meshRenderer = mainEntity.AddComponent<MeshRendererComponent>();
        //meshRenderer.model(cubeModel);
        meshRenderer.model(AssetManager::Get().LoadModel(
            "res/models/backpack/backpack.obj",
            AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/StandDiffuse.glsl")
        ));
        meshRenderer.subMeshIndex(-1);
        mainEntity.AddComponent<ScriptComponent>().AddScript<RotateScript>();

        light = scene->AddEntity("Directional Light");
        LightComponent& lightComponent = light.AddComponent<LightComponent>();
        lightComponent.color = Vector3(1,1,1);
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

        for(int i = 0; i < 2500; i++){
            float posRange = 20;

            Entity e = scene->AddEntity("Entity" + std::to_string(random(0, 200)));
            MeshRendererComponent& mr = e.AddComponent<MeshRendererComponent>();
            mr.model(cubeModel);
            mr.materialsOverride()[0] = AssetManager::Get().LoadMaterial("res/textures/floor.material");
        
            float angle = 20.0f * i; 
            e.GetComponent<TransformComponent>().localPosition(Vector3(random(-posRange, posRange), random(0, posRange), random(-posRange, posRange)));
            e.GetComponent<TransformComponent>().localEulerAngles(Vector3(random(-180, 180), random(-180, 180), random(-180, 180)));
            otherEntity = e;

            scene->SetParent(mainEntity.id(), e.id());
        }

        for(int i = 0; i < 0; i++){
            float posRange = 100;
            AddCharacter(Vector3(random(-posRange, posRange), random(0, posRange), random(-posRange, posRange)));
        }

        Application::AddModule<Editor>();
    }

    void OnUpdate(float deltaTime) override {
        //camMove.OnUpdate();

        //mainEntity.GetComponent<TransformComponent>().localEulerAngles(Vector3(0, Platform::GetTime() * 40, 0));
        //otherEntity.GetComponent<TransformComponent>().position(Vector3(-5, 2, -1.5f));

        scene->Update();
    }   

    void OnRender(float deltaTime) override {
        //Renderer::SetDepthTest(DepthTest::LESS);
        //Renderer::SetCullFace(CullFace::BACK);

        scene->Draw();

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