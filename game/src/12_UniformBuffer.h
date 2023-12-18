#pragma once

#include <OD/OD.h>
#include <OD/Renderer/UniformBuffer.h>
#include "CameraMovement.h"
#include "Ultis.h"

using namespace OD;

struct UniformBuffer_12: OD::Module {
    Ref<Model> model;
    Ref<Shader> shader;
    UniformBuffer cBuffer;

    Transform camTransform;
    Camera cam;
    CameraMovement camMove;

    std::vector<Matrix4> transforms;

    void OnInit() override {
        LogInfo("Game Init");

        Application::Vsync(false);

        camTransform.LocalPosition(Vector3(0, 2, 4));
        camTransform.LocalEulerAngles(Vector3(-25, 0, 0));

        camMove.transform = &camTransform;

        UniformBuffer::Create(cBuffer);

        model = AssetManager::Get().LoadModel("res/Game/Models/cube.glb");
        model->materials[0]->SetShader(AssetManager::Get().LoadShaderFromFile("res/Game/Shaders/UniformBufferInstancing.glsl"));
        model->materials[0]->SetTexture("mainTex", AssetManager::Get().LoadTexture2D("res/Game/Textures/rock.jpg", {OD::TextureFilter::Linear, false}));

        for(int i = 0; i < 100; i++){
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

        Renderer::Begin();
        Renderer::Clean(0.1f, 0.1f, 0.1f, 1);
        Renderer::SetCamera(cam);

        model->materials[0]->UpdateDatas();

        model->meshs[0]->instancingModelMatrixs.clear();
        for(auto i: transforms){
            model->meshs[0]->instancingModelMatrixs.push_back(i);
        }
        model->meshs[0]->UpdateMeshInstancingModelMatrixs();

        cBuffer.SetData(&transforms[0], sizeof(Matrix4) * transforms.size(), 0);
        model->materials[0]->GetShader()->SetUniforBuffer("Model", cBuffer, 0);
        
        Renderer::SetDefaultShaderData(*model->materials[0]->GetShader(), Matrix4Identity, true);
        Renderer::DrawMeshInstancing(*model->meshs[0], transforms.size());
        
        Renderer::End();
    }

    void OnGUI() override {
        ImGui::Begin("Renderer Stats");
        ImGui::Text("DrawCalls: %d", Renderer::drawCalls);
        ImGui::Text("Vertices: %dk", Renderer::vertices / 1000);
        ImGui::Text("Tris: %dk", Renderer::tris / 1000);
        ImGui::End();
    }

    void OnResize(int width, int height) override {;
    }
};