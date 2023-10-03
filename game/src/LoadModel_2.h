#pragma once

#include <OD/OD.h>
#include "CameraMovement.h"
#include "Ultis.h"

using namespace OD;

struct LoadModel_2: OD::Module {
    Ref<Model> model;
    Ref<Shader> shader;
    Transform camTransform;
    Camera cam;
    CameraMovement camMove;

    std::vector<Matrix4> transforms;

    void OnInit() override {
        LogInfo("Game Init");

        Application::vsync(false);

        camTransform.localPosition(Vector3(0, 2, 4));
        camTransform.localEulerAngles(Vector3(-25, 0, 0));

        camMove.transform = &camTransform;

        model = AssetManager::Get().LoadModel("res/models/cube.glb");
        model->materials[0]->shader(AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/Unlit.glsl"));
        model->materials[0]->SetTexture("mainTex", AssetManager::Get().LoadTexture2D("res/textures/rock.jpg", OD::TextureFilter::Linear, false));

        for(int i = 0; i < 100000; i++){
            float posRange = 25;

            Transform t;

            float angle = 20.0f * i; 
            t.localPosition(Vector3(random(-posRange, posRange), random(0, posRange), random(-posRange, posRange)));
            t.localEulerAngles(Vector3(random(-180, 180), random(-180, 180), random(-180, 180)));

            transforms.push_back(t.GetLocalModelMatrix());
        }
    }

    void OnUpdate(float deltaTime) override {
        camMove.OnUpdate();
        //modelTransform.localEulerAngles(Vector3(0, Platform::GetTime() * 20, 0));
    }   

    void OnRender(float deltaTime) override {
        OD_PROFILE_SCOPE("LoadModel_2::OnRender");

        cam.SetPerspective(60, 0.1f, 1000.0f, Application::screenWidth(), Application::screenHeight());
        cam.view = camTransform.GetLocalModelMatrix().inverse();

        Renderer::Begin();
        Renderer::Clean(0.1f, 0.1f, 0.1f, 1);
        Renderer::SetCamera(cam);

        //Renderer::SetRenderMode(Renderer::RenderMode::WIREFRAME);
        //Renderer::DrawModel(*model, modelTransform.GetLocalModelMatrix());

        bool useInstancing = true;

        if(useInstancing){  
            model->materials[0]->UpdateUniforms();

            model->meshs[0]->instancingModelMatrixs.clear();
            for(auto i: transforms){
                model->meshs[0]->instancingModelMatrixs.push_back(i);
            }
            model->meshs[0]->UpdateMeshInstancingModelMatrixs();

            Renderer::DrawMeshInstancing(*model->meshs[0], *model->materials[0]->shader(), transforms.size());
        } else {
            for(auto i: transforms){
                Renderer::DrawModel(*model, i);
            }
        }
        
        /*for(int i = 0; i < 10; i++){
            modelTransform.localPosition(cubePositions[i]);
            float angle = 20.0f * i; 
            modelTransform.localEulerAngles(Vector3(angle*1, angle*0.3f, angle*0.5f));
            Renderer::DrawModel(*model, modelTransform.GetLocalModelMatrix());
        }*/
        

        Renderer::End();
    }

    void OnGUI() override {
        //static bool show;
        //ImGui::ShowDemoWindow(&show);

        ImGui::Begin("Renderer Stats");
        ImGui::Text("DrawCalls: %d", Renderer::drawCalls);
        ImGui::Text("Vertices: %dk", Renderer::vertices / 1000);
        ImGui::Text("Tris: %dk", Renderer::tris / 1000);
        ImGui::End();
    }

    void OnResize(int width, int height) override {;
    }
};