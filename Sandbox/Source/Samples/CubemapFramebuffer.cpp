#include "OD/pch.h"
#include "CubemapFramebuffer.h"
#include "Ultis/Ultis.h"
#include <OD/Core/Application.h>
#include <OD/Core/Instrumentor.h>
#include <OD/Core/ImGui.h>
#include <OD/Graphics/Mesh.h>
#include <OD/Graphics/Model.h>
#include <OD/Graphics/Cubemap.h>
#include <OD/Graphics/Material.h>
#include <OD/Graphics/Texture.h>
#include <OD/Graphics/Framebuffer.h>
#include <OD/Graphics/Ultis.h>
#include <OD/Graphics/Graphics.h>

void CubemapFramebufferSample::OnInit(){
    LogInfo("Game Init");

    Application::Vsync(false);

    camTransform.Position(Vector3(0, 0, 10));
    camTransform.EulerAngles(Vector3(0, 0, 0));
    camMove.transform = &camTransform;

    mat1 = ResourceManager::Get().Create<Material>(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/Unlit.glsl"));
    mat1->SetTexture("mainTex", ResourceManager::Get().LoadByPath<Texture2D>("Sandbox/Textures/Rock.jpg"));
    mat1->SetVector4("color", {1,1,1,1});

    mat2 = ResourceManager::Get().Create<Material>(ResourceManager::Get().LoadByPath<Shader>("Sandbox/Shaders/CubemapFramebufferSample.glsl"));

    model = ResourceManager::Get().LoadByPath<Model>("Sandbox/Models/cube.gltf");
    model2 = ResourceManager::Get().LoadByPath<Model>("Engine/Models/Sphere.obj");

    FrameBufferSpecification tempSp;
    tempSp.width = Application::ScreenHeight();
    tempSp.height = Application::ScreenWidth();
    tempSp.type = FramebufferAttachmentType::TEXTURE_2D_MULTISAMPLE;
    tempSp.colorAttachments = {{FramebufferTextureFormat::RGBA8}};
    tempSp.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT24};
    tempSp.sample = 8;
    tempFBMultsample = CreateRef<Framebuffer>(tempSp);
    tempSp.type = FramebufferAttachmentType::TEXTURE_2D;
    tempSp.createDepth = false;
    tempFB = CreateRef<Framebuffer>(tempSp);
    
    FrameBufferSpecification sp;
    sp.width = 256*1;
    sp.height = 256*1;
    sp.type = FramebufferAttachmentType::CUBEMAP;
    sp.colorAttachments = {{FramebufferTextureFormat::RGBA8, true, CalculateMipCount(sp.width, sp.height)}};
    sp.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT24};
    cubeFB = CreateRef<Framebuffer>(sp);

    skyMesh = Mesh::SkyboxCube();
    skyMat = ResourceManager::Get().Create<Material>(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/SkyboxCubemap.glsl"));
    skyMat->SetCubemap("mainTex", ResourceManager::Get().LoadByPath<Cubemap>("DefaultSkyboxCubemap"));

    fullscreenQuadMesh = Mesh::FullScreenQuad();
    screenPassMat = ResourceManager::Get().Create<Material>(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/GamaCorrectionPP.glsl"));
}

void CubemapFramebufferSample::OnUpdate(float deltaTime){
    camMove.OnUpdate();
    //modelTransform.localEulerAngles(Vector3(0, Platform::GetTime() * 20, 0));
}   

void CubemapFramebufferSample::OnRender(float deltaTime){
    OD_PROFILE_SCOPE("LoadModel_2::OnRender");

    auto DrawSky = [&](Matrix4 view){
        //Matrix4 skyboxView = Matrix4(glm::mat4(glm::mat3(cam.view)));
        skyMat->SetMatrix4("skyboxView", view);
        Graphics::DrawMesh(*skyMesh, *skyMat, Matrix4Identity);
    };

    auto DrawScene1 = [&](){
        Graphics::DrawMesh(*model->meshs[0], *mat1, Transform({-4, 0, 0}).GetModelMatrix());
        Graphics::DrawMesh(*model->meshs[0], *mat1, Transform({4, 0, 0}).GetModelMatrix());
    };

    auto DrawScene2 = [&](){
        mat2->SetTexture("mainTex", cubeFB.get(), 0);
        mat2->SetFloat("roughness", roughness);
        mat2->SetFloat("metalness", metalness);
        mat2->SetFloat("levels", cubeFB->Specification().colorAttachments[0].mipLevels);
        mat2->SetVector3("cameraPos", camTransform.Position());

        Graphics::DrawMesh(*model->meshs[0], *mat1, Transform({-4, 0, 0}).GetModelMatrix());
        Graphics::DrawMesh(*model->meshs[0], *mat1, Transform({4, 0, 0}).GetModelMatrix());
        Graphics::DrawMesh(*model2->meshs[0], *mat2, Transform({0, 0, 0}).GetModelMatrix());
    };

    auto RenderCubemap = [&](Vector3 position){
        const float nearPlane = 0.1f;
        const float farPlane  = 1000.0f;

        Matrix4 proj = math::perspective(
            math::radians(90.0f),
            1.0f, //static_cast<float>(Application::ScreenWidth()) / static_cast<float>(Application::ScreenHeight()),
            nearPlane,
            farPlane
        );

        Matrix4 views[6] ={
            math::lookAt(position, position + Vector3(1,0,0),  Vector3(0,-1,0)),
            math::lookAt(position, position + Vector3(-1,0,0), Vector3(0,-1,0)),
            math::lookAt(position, position + Vector3(0,1,0),  Vector3(0,0,1)),
            math::lookAt(position, position + Vector3(0,-1,0), Vector3(0,0,-1)),
            math::lookAt(position, position + Vector3(0,0,1),  Vector3(0,-1,0)),
            math::lookAt(position, position + Vector3(0,0,-1), Vector3(0,-1,0))
        };

        for(int face = 0; face < 6; face++){
            Camera cam;
            cam.projection = proj;
            cam.view = views[face];

            Graphics::BeginFramebuffer(*cubeFB, true, {0,0,0,1}, face, 0);
            Graphics::SetViewport(0, 0, cubeFB->Specification().width, cubeFB->Specification().height);

            Graphics::Clean(0.1f, 0.1f, 0.1f, 1);
            Graphics::SetCamera(cam);

            DrawScene1();
            DrawSky(Matrix4(glm::mat4(glm::mat3(cam.view))));

            Graphics::EndFramebuffer();
        }

        // generate mipmaps if enabled
        cubeFB->GenMipmap();
    };

    auto RenderMainScene = [&](){
        cam.SetPerspective(60, 0.1f, 1000.0f, Application::ScreenWidth(), Application::ScreenHeight());
        cam.view = math::inverse(camTransform.GetModelMatrix());

        tempFB->Resize(Application::ScreenWidth(), Application::ScreenHeight());
        tempFBMultsample->Resize(Application::ScreenWidth(), Application::ScreenHeight());
        
        Graphics::BeginFramebuffer(*tempFBMultsample, true, {0,0,0,1});
        Graphics::SetViewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
        Graphics::Clean(0.1f, 0.1f, 0.1f, 1);
        Graphics::SetCamera(cam);
        
        DrawScene2();
        DrawSky(Matrix4(glm::mat4(glm::mat3(cam.view))));

        Graphics::EndFramebuffer();

        Graphics::BlitFramebuffer(tempFBMultsample.get(), tempFB.get(), 0);
    };

    auto DrawToFinal = [&](){
        Graphics::BeginRenderToScreen();
        Graphics::SetViewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
        Graphics::Clean(0.1f, 0.1f, 0.1f, 1);   

        screenPassMat->SetTexture("mainTex", tempFB.get(), 0);
        Graphics::DrawMesh(*fullscreenQuadMesh, *screenPassMat, Matrix4Identity);

        Graphics::EndRenderToScreen();
    };

    Graphics::Begin();

    RenderCubemap({0, 0, 0});
    RenderMainScene();
    DrawToFinal();

    Graphics::End();
}

void CubemapFramebufferSample::OnGUI(){
    //static bool show;
    //ImGui::ShowDemoWindow(&show);

    ImGui::Begin("CubemapFramebufferSample");
    ImGui::DragFloat("roughness", &roughness, 0.1f, 0, 1);
    ImGui::DragFloat("metalness", &metalness, 0.1f, 0, 1);
    ImGui::End();
}

void CubemapFramebufferSample::OnResize(int width, int height){}
void CubemapFramebufferSample::OnExit(){}