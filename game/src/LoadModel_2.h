#pragma once

#include <OD/OD.h>
#include "CameraMovement.h"

using namespace OD;

struct LoadModel_2: OD::Module {
    Ref<Model> model;
    Ref<Shader> shader;
    Transform modelTransform;
    Transform camTransform;
    Camera cam;
    CameraMovement camMove;

    Vector3 cubePositions[10] = {
        Vector3( 0.0f,  0.0f,  0.0f), 
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

        modelTransform.localPosition(Vector3::zero);
        camTransform.localPosition(Vector3(0, 2, 4));
        camTransform.localEulerAngles(Vector3(-25, 0, 0));

        //model = Model::CreateFromFile("res/models/suzane.obj");
        //model->SetShader(Shader::CreateFromFile("res/shaders/model.glsl"));
        //model->materials[0].SetTexture("texture1", Texture2D::CreateFromFile("res/textures/rock.jpg", false, OD::TextureFilter::Linear, false));

        model = AssetManager::Get().LoadModel("res/models/suzane.obj");
        model->materials[0]->shader = AssetManager::Get().LoadShaderFromFile("res/shaders/model.glsl");
        model->materials[0]->SetTexture("texture1", AssetManager::Get().LoadTexture2D("res/textures/rock.jpg", OD::TextureFilter::Linear, false));
    }

    void OnUpdate(float deltaTime) override {
        camMove.OnUpdate();
        modelTransform.localEulerAngles(Vector3(0, Platform::GetTime() * 20, 0));
    }   

    void OnRender(float deltaTime) override {
        cam.SetPerspective(60, 0.1f, 1000.0f, Application::screenWidth(), Application::screenHeight());
        cam.view = camTransform.GetLocalModelMatrix().inverse();

        Renderer::Begin();
        Renderer::Clean(0.1f, 0.1f, 0.1f, 1);
        Renderer::SetCamera(cam);

        //Renderer::SetRenderMode(Renderer::RenderMode::WIREFRAME);
        //Renderer::DrawModel(*model, modelTransform.GetLocalModelMatrix());

        
        for(unsigned int i = 0; i < 10; i++){
            modelTransform.localPosition(cubePositions[i]);
            float angle = 20.0f * i; 
            modelTransform.localEulerAngles(Vector3(angle*1, angle*0.3f, angle*0.5f));
            Renderer::DrawModel(*model, modelTransform.GetLocalModelMatrix());
        }
        

        Renderer::End();
    }

    void OnGUI() override {
        //static bool show;
        //ImGui::ShowDemoWindow(&show);
    }

    void OnResize(int width, int height) override {;
    }
};