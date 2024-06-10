#pragma once

#include <OD/OD.h>
#include "Ultis/CameraMovement.h"
#include "Ultis/Ultis.h"

using namespace OD;

struct LoadModel_2: OD::Module {
    Ref<Model> model;
    Ref<Shader> shader;
    Transform camTransform;
    Camera cam;
    CameraMovement camMove;
    std::vector<Matrix4> transforms;

    bool useInstancing = true;

    void OnInit() override {
        LogInfo("Game Init");

        Application::Vsync(false);

        camTransform.LocalPosition(Vector3(0, 10, 70));
        camTransform.LocalEulerAngles(Vector3(0, 0, 0));

        camMove.transform = &camTransform;

        model = AssetManager::Get().LoadAsset<Model>("res/Game/Models/cube.glb");
        model->materials[0]->SetShader(AssetManager::Get().LoadAsset<Shader>("res/Engine/Shaders/Unlit.glsl"));
        model->materials[0]->SetTexture("mainTex", AssetManager::Get().LoadAsset<Texture2D>("res/Game/Textures/rock.jpg"));

        for(int i = 0; i < 100000; i++){
            float posRange = 25;

            Transform t;

            float angle = 20.0f * i; 
            t.LocalPosition(Vector3(random(-posRange, posRange), random(0, posRange), random(-posRange, posRange)));
            t.LocalEulerAngles(Vector3(random(-180, 180), random(-180, 180), random(-180, 180)));

            transforms.push_back(t.GetLocalModelMatrix());
        }
    }

    void OnUpdate(float deltaTime) override {
        camMove.OnUpdate();
        //modelTransform.localEulerAngles(Vector3(0, Platform::GetTime() * 20, 0));
    }   

    void OnRender(float deltaTime) override {
        OD_PROFILE_SCOPE("LoadModel_2::OnRender");

        cam.SetPerspective(60, 0.1f, 1000.0f, Application::ScreenWidth(), Application::ScreenHeight());
        cam.view = math::inverse(camTransform.GetLocalModelMatrix());

        Graphics::Begin();
        Graphics::Clean(0.1f, 0.1f, 0.1f, 1);
        Graphics::SetCamera(cam);

        //Renderer::SetRenderMode(Renderer::RenderMode::WIREFRAME);
        //Renderer::DrawModel(*model, modelTransform.GetLocalModelMatrix());

        if(useInstancing){  
            //model->materials[0]->SetEnableInstancing(true);
            model->materials[0]->DisableKeyword("SKINNED");
            model->materials[0]->EnableKeyword("INSTANCING");
            Material::SubmitGraphicDatas(*model->materials[0]);

            /*
            model->meshs[0]->instancingModelMatrixs.clear();
            for(auto i: transforms){
                model->meshs[0]->instancingModelMatrixs.push_back(i);
            }
            model->meshs[0]->UpdateMeshInstancingModelMatrixs();*/

            //Graphics::SetDefaultShaderData(*model->materials[0]->GetShader(), Matrix4Identity, true);
            /*Shader::Bind(*model->materials[0]->GetShader());
            Graphics::SetProjectionViewMatrix(*model->materials[0]->GetShader());
            Graphics::DrawMeshInstancingRaw(*model->meshs[0], transforms.size());*/
            
            Graphics::DrawMeshInstancing(*model->meshs[0], *model->materials[0]->GetShader(), &transforms[0], transforms.size());

        } else {
            model->materials[0]->DisableKeyword("SKINNED");
            model->materials[0]->DisableKeyword("INSTANCING");
            Material::SubmitGraphicDatas(*model->materials[0]);

            for(auto i: transforms){
                //Graphics::SetDefaultShaderData(*model->materials[0]->GetShader(), i, false);
                /*Shader::Bind(*model->materials[0]->GetShader());
                Graphics::SetProjectionViewMatrix(*model->materials[0]->GetShader());
                Graphics::SetModelMatrix(*model->materials[0]->GetShader(), i);
                Graphics::DrawMeshRaw(*model->meshs[0]);*/
                Graphics::DrawMesh(*model->meshs[0], *model->materials[0]->GetShader(), i);
            }
        }
        
        /*for(int i = 0; i < 10; i++){
            modelTransform.localPosition(cubePositions[i]);
            float angle = 20.0f * i; 
            modelTransform.localEulerAngles(Vector3(angle*1, angle*0.3f, angle*0.5f));
            Renderer::DrawModel(*model, modelTransform.GetLocalModelMatrix());
        }*/
        

        Graphics::End();
    }

    void OnGUI() override {
        //static bool show;
        //ImGui::ShowDemoWindow(&show);

        ImGui::Begin("Load Model Test");

        ImGui::Checkbox("Use Instancing", &useInstancing);
        ImGui::Spacing();
        
        ImGui::Text("DrawCalls: %d", Graphics::GetDrawCallsCount());
        //ImGui::Text("Vertices: %dk", Graphics::GetVerticesCount() / 1000);
        //ImGui::Text("Tris: %dk", Graphics::GetTrisCount() / 1000);
        
        if(Graphics::GetVerticesCount() >= 1000000){
            ImGui::Text("Vertices: %.1fM", Graphics::GetVerticesCount() / 1000000.0f);
        } else if(Graphics::GetVerticesCount() >= 1000){
            ImGui::Text("Vertices: %.1fk", Graphics::GetVerticesCount() / 1000.0f);
        } else {
            ImGui::Text("Vertices: %d", Graphics::GetVerticesCount());
        }

        if(Graphics::GetTrisCount() >= 1000000){
            ImGui::Text("Tris: %.1fM", Graphics::GetTrisCount() / 1000000.0f);
        } else if(Graphics::GetTrisCount() >= 1000){
            ImGui::Text("Tris: %.1fk", Graphics::GetTrisCount() / 1000.0f);
        } else {
            ImGui::Text("Tris: %d", Graphics::GetTrisCount());
        }
        
        ImGui::End();
    }

    void OnResize(int width, int height) override {}
    void OnExit() override {}
    
};