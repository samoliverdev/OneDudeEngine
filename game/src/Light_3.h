#pragma once

#include <OD/OD.h>
#include "CameraMovement.h"
#include <assert.h>

using namespace OD;

struct Light_3: OD::Module {
    Ref<Model> model;
    Ref<Shader> shader;
    Transform modelTransform;
    Transform camTransform;
    Camera cam;
    //CameraMovement camMove;

    Ref<Model> lightModel;
    Transform lightTransform;

    Vector3 cubePositions[10] = {
        //Vector3( 0.0f,  0.0f,  0.0f), 
        Vector3( 2.0f,  5.0f, -15.0f), 
        Vector3(-1.5f, -2.2f, -2.5f),  
        Vector3(-3.8f, -2.0f, -12.3f),  
        Vector3( 2.4f, -0.4f, -3.5f),  
        Vector3(-1.7f,  3.0f, -7.5f),  
        Vector3( 1.3f, -2.0f, -2.5f),  
        Vector3( 1.5f,  2.0f, -2.5f), 
        Vector3( 1.5f,  0.2f, -1.5f), 
        Vector3(-1.3f,  1.0f, -1.5f)  
    };

    void OnInit() override {
        LogInfo("Game Init");
        //LogInfo("SizeOf glm::vec3: %zu", sizeof(glm::vec3));
        //LogInfo("SizeOf Vector3 %zu", sizeof(Vector3));
        //LogInfo("(%f, %f, %f)", (Vector3(1,1,1)+Vector3(2,2,2)).x, (Vector3(1,1,1) * 2.0f).y, (glm::vec3(1,1,1) * 5.0f).z);

        lightModel = AssetManager::Get().LoadModel("res/models/sphere.obj");
        lightModel->materials[0]->shader = AssetManager::Get().LoadShaderFromFile("res/shaders/UnlitColor.glsl");
        lightModel->materials[0]->SetVector3("color", Vector3(1,1,1));
        lightTransform.localScale(Vector3(0.1f, 0.1f, 0.1f));
        lightTransform.localPosition(Vector3(-1, 2, 2));

        //camMove.transform = &camTransform;

        modelTransform.localPosition(Vector3::zero);
        camTransform.localPosition(Vector3(0, 2, 4));
        camTransform.localEulerAngles(Vector3(-25, 0, 0));

        //model = Model::CreateFromFile("res/models/suzane.obj");
        //model->SetShader(Shader::CreateFromFile("res/shaders/model.glsl"));
        //model->materials[0].SetTexture("texture1", Texture2D::CreateFromFile("res/textures/rock.jpg", false, OD::TextureFilter::Linear, false));

        model = AssetManager::Get().LoadModel("res/models/suzane.obj");
        model->materials[0]->shader = AssetManager::Get().LoadShaderFromFile("res/shaders/light.glsl");
        model->materials[0]->SetTexture("texture1", AssetManager::Get().LoadTexture2D("res/textures/rock.jpg", OD::TextureFilter::Linear, false));
        model->materials[0]->SetVector3("color", Vector3(1.0f, 0.5f, 0.31f));
        model->materials[0]->SetVector3("lightColor", Vector3(1.0f, 1.0f, 1.0f));
        model->materials[0]->SetVector3("light.position", lightTransform.localPosition());
        model->materials[0]->SetVector3("viewPos", camTransform.localPosition());
        model->materials[0]->SetVector3("material.ambient", Vector3(0.3f, 0.3f, 0.31f));
        model->materials[0]->SetVector3("material.diffuse", Vector3(0.8f, 0.8f, 0.31f));
        model->materials[0]->SetVector3("material.specular", Vector3(0.5f, 0.5f, 0.5f));
        model->materials[0]->SetFloat("material.shininess", 32.0f);
        model->materials[0]->SetVector3("light.ambient",  Vector3(0.2f, 0.2f, 0.2f));
        model->materials[0]->SetVector3("light.diffuse",  Vector3(0.5f, 0.5f, 0.5f)); // darken diffuse light a bit
        model->materials[0]->SetVector3("light.specular", Vector3(1.0f, 1.0f, 1.0f)); 
    }

    void OnUpdate(float deltaTime) override {
        //camMove.OnUpdate();
        modelTransform.localEulerAngles(Vector3(0, Platform::GetTime() * 40, 0));
    }   

    void OnRender(float deltaTime) override {
        cam.SetPerspective(60, 0.1f, 1000.0f, Application::screenWidth(), Application::screenHeight());
        cam.view = camTransform.GetLocalModelMatrix().inverse();

        model->materials[0]->SetVector3("viewPos", camTransform.localPosition());

        Renderer::Begin();
        Renderer::Clean(0.1f, 0.1f, 0.1f, 1);
        Renderer::SetCamera(cam);

        //Renderer::SetRenderMode(Renderer::RenderMode::WIREFRAME);
        Renderer::DrawModel(*model, modelTransform.GetLocalModelMatrix());
        Renderer::DrawModel(*lightModel, lightTransform.GetLocalModelMatrix());

        ///*
        for(unsigned int i = 0; i < 10; i++){
            modelTransform.localPosition(cubePositions[i]);
            float angle = 20.0f * i; 
            modelTransform.localEulerAngles(Vector3(angle*1, angle*0.3f, angle*0.5f));
            Renderer::DrawModel(*model, modelTransform.GetLocalModelMatrix());
        }
        //*/
        

        Renderer::End();
    }

    void OnGUI() override {
        ImGui::Begin("Triangle Position/Color");

        static float translation[] = {0.0, 0.0, 0.0};
        ImGui::SliderFloat3("position", translation, -5, 5);
        lightTransform.localPosition(Vector3(translation[0], translation[1], translation[2]));

        static float color[4] = { 1.0f,1.0f,1.0f,1.0f };
        ImGui::ColorEdit3("color", color);
        
        model->materials[0]->SetVector3("light.diffuse",  Vector3(color[0], color[1], color[2]));
        model->materials[0]->SetVector3("light.position", lightTransform.localPosition());
        
        ImGui::End();
    }

    void OnResize(int width, int height) override {;
    }
};