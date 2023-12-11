#pragma once

#include <OD/OD.h>
#include "CameraMovement.h"
#include <assert.h>
#include "Ultis.h"

using namespace OD;

struct SynthCity_10: OD::Module {
    //CameraMovement camMove;
    Entity camera;

    void OnInit() override {
        LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

        Application::Vsync(false);

        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

        Scene* scene = SceneManager::Get().NewScene();

        Entity env = scene->AddEntity("Env");
        env.AddComponent<EnvironmentComponent>().settings.ambient = Vector3(0.11f,0.16f,0.25f);

        Entity light = scene->AddEntity("Light");
        LightComponent& lightComponent = light.AddComponent<LightComponent>();
        lightComponent.color = Vector3(1,1,1);
        light.GetComponent<TransformComponent>().Position(Vector3(-2, 4, -1));
        light.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(45, -125, 0));
        lightComponent.renderShadow = true;

        camera = scene->AddEntity("Camera");
        CameraComponent& cam = camera.AddComponent<CameraComponent>();
        camera.GetComponent<TransformComponent>().LocalPosition(Vector3(0, 15, 15));
        camera.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(-25, 0, 0));
        camera.AddComponent<ScriptComponent>().AddScript<CameraMovementScript>()->moveSpeed = 60;
        cam.farClipPlane = 1000;

        Ref<Model> floorModel = AssetManager::Get().LoadModel(
            "res/Game/Models/PolygonCity/FBX_SCENE/City.fbx",
            //"res/PolygonCity/City.fbx",
            AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/StandDiffuse.glsl")
        );
  
        Entity floorEntity = scene->AddEntity("City");
        MeshRendererComponent& floorRenderer = floorEntity.AddComponent<MeshRendererComponent>();
        floorRenderer.SetModel(floorModel);
        TransformComponent& cityTransform = floorEntity.GetComponent<TransformComponent>();
        cityTransform.LocalScale(Vector3(0.01f, 0.01f, 0.01f));
    
        Application::AddModule<Editor>();
        //scene->Start();
    }

    void OnUpdate(float deltaTime) override {
        SceneManager::Get().ActiveScene()->Update();
    }   

    void OnRender(float deltaTime) override {
        SceneManager::Get().ActiveScene()->Draw();
    }

    void OnGUI() override {
        
        /*
        if(ImGui::Begin("Profile")){
            for(auto i: Instrumentor::Get().results()){ 
                float durration = (i.end - i.start) * 0.001f;
                ImGui::Text("%s: %.3f.ms", i.name, durration);
            }
            Instrumentor::Get().results().clear();
        }
        ImGui::End();
        */

    }
    void OnResize(int width, int height) override {}
};