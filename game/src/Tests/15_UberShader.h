#pragma once

#include <OD/OD.h>
#include <OD/Graphics/ShaderHandler.h>
#include "Ultis/CameraMovement.h"
#include <assert.h>

using namespace OD;

struct UberShader_15: OD::Module {
    Ref<Model> model;
    //Ref<Shader> shader;
    ShaderHandler* uberShader;
    Transform modelTransform;
    Transform camTransform;
    Camera cam;
    CameraMovement camMove;

    void OnInit() override {
        LogInfo("Game Init");

        modelTransform.LocalPosition(Vector3Zero);
        camTransform.LocalPosition(Vector3(0, 2, 10));
        camTransform.LocalEulerAngles(Vector3(-25, 0, 0));

        camMove.transform = &camTransform;

        model = AssetManager::Get().LoadModel("res/Game/Models/suzane.obj");
        uberShader = new ShaderHandler("res/Game/Shaders/UberShaderTest.glsl");
        uberShader->EnableKeyword("Fade");
        uberShader->EnableKeyword("Instancing");
    }

    void OnUpdate(float deltaTime) override {
        camMove.OnUpdate();
        //modelTransform.LocalEulerAngles(Vector3(0, Platform::GetTime() * 40, 0));
    }   

    void OnRender(float deltaTime) override {
        cam.SetPerspective(60, 0.1f, 1000.0f, Application::ScreenWidth(), Application::ScreenHeight());
        cam.view = math::inverse(camTransform.GetLocalModelMatrix());

        Graphics::Begin();
        Graphics::Clean(0.1f, 0.1f, 0.1f, 1);
        Graphics::SetCamera(cam);

        Matrix4 m1 = math::translate(Matrix4Identity, Vector3(2, 0, 0));
        Matrix4 m2 = math::translate(Matrix4Identity, Vector3(-2, 0, 0));

        uberShader->EnableKeyword("COLOR_1");
        uberShader->SetCurrentShader();
        //Graphics::SetDefaultShaderData(*uberShader->GetCurrentShader(), m1);
        Shader::Bind(*uberShader->GetCurrentShader());
        Graphics::SetProjectionViewMatrix(*uberShader->GetCurrentShader());
        Graphics::SetModelMatrix(*uberShader->GetCurrentShader(), m1);
        Graphics::DrawMeshRaw(*model->meshs[0]);

        uberShader->DisableKeyword("COLOR_1");
        uberShader->SetCurrentShader();
        //Graphics::SetDefaultShaderData(*uberShader->GetCurrentShader(), m2);
        Shader::Bind(*uberShader->GetCurrentShader());
        Graphics::SetProjectionViewMatrix(*uberShader->GetCurrentShader());
        Graphics::SetModelMatrix(*uberShader->GetCurrentShader(), m2);
        Graphics::DrawMeshRaw(*model->meshs[0]);

        Graphics::End();
    }

    void OnGUI() override {}
    void OnResize(int width, int height) override {}
    void OnExit() override {}
};