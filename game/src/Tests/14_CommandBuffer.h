#pragma once

#include <OD/OD.h>
#include <OD/RenderPipeline/CommandBuffer.h>
#include <chrono>
#include <functional>
#include "Ultis/CameraMovement.h"
#include "Ultis/Ultis.h"

using namespace OD;

struct CommandBuffer_14: OD::Module {
    Ref<Model> cubeModel;
    Ref<Shader> meshShader;
    Ref<Shader> fontShader;
    Ref<Font> font;
    Ref<Texture2D> texture;
    Ref<Material> mat;

    Camera cam;
    Transform camTransform;
    CameraMovement camMove;

    CommandBuffer cmd1;
    CommandBuffer cmd2;

    Framebuffer* renderTarget;

    void OnInit() override {
        LogInfo("Game Init");
        Assert(false && "Outdate");

        camTransform.LocalPosition(Vector3(0, 2, 4));
        camTransform.LocalEulerAngles(Vector3(-25, 0, 0));
        camMove.transform = &camTransform;

        FrameBufferSpecification framebufferSpecification = {Application::ScreenWidth(), Application::ScreenHeight()};
        framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RGBA8}};
        framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8};
        framebufferSpecification.type = FramebufferAttachmentType::TEXTURE_2D;
        renderTarget = new Framebuffer(framebufferSpecification);

        cubeModel = AssetManager::Get().LoadAsset<Model>("res/Game/Models/cube.glb");

        meshShader = Shader::CreateFromFile("res/Engine/Shaders/Unlit.glsl");
        texture = Texture2D::CreateFromFile("res/Game/Textures/image.jpg", Texture2DSetting());

        mat = CreateRef<Material>();
        mat->SetShader(meshShader);
        //mat->SetTexture("mainTex", texture);

        fontShader = Shader::CreateFromFile("res/Engine/Shaders/Font.glsl");

        font = Font::CreateFromFile("res/Engine/Fonts/OpenSans/static/OpenSans_Condensed-Bold.ttf");
    }

    void OnUpdate(float deltaTime) override {
        camMove.OnUpdate();
    }   

    void OnRender(float deltaTime) override {

        cam.SetPerspective(60, 0.1f, 1000.0f, Application::ScreenWidth(), Application::ScreenHeight());
        cam.view = math::inverse(camTransform.GetLocalModelMatrix());

        Graphics::Begin();

        cmd1.SetRenderTarget(renderTarget);
        cmd1.SetCamera(cam);
        cmd1.CleanRenderTarget({0.1f, 0.1f, 1.0f});
        //cmd1.AddGlobalShader(meshShader);
        //cmd1.SetGlobalMatrix4("view", cam.view);
        //cmd1.SetGlobalMatrix4("projection", cam.projection);
        //cmd1.SetGlobalTexture("mainTex", texture);
        cmd1.AddDrawCommand({
            mat,
            cubeModel->meshs[0],
            Matrix4Identity
        });
        
        cmd2.SetRenderTarget(nullptr);
        cmd2.SetCamera(cam);
        cmd2.CleanRenderTarget({0.1f, 0.1f, 0.1f});
        //cmd2.AddGlobalShader(meshShader);
        //cmd2.SetGlobalMatrix4("view", cam.view);
        //cmd2.SetGlobalMatrix4("projection", cam.projection);
        //cmd2.SetGlobalTexture("mainTex", renderTarget);
        cmd2.AddDrawCommand({
            mat,
            cubeModel->meshs[0],
            Matrix4Identity
        });

        cmd1.Submit();
        cmd2.Submit();

        cmd1.Clean();
        cmd2.Clean();

        Graphics::End();
    }

    void OnGUI() override {
        //static bool show;
        //ImGui::ShowDemoWindow(&show);
    }

    void OnResize(int width, int height) override {}
    void OnExit() override {}
};