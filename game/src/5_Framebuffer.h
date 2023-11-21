#pragma once

#include <OD/OD.h>
#include "CameraMovement.h"
#include <assert.h>

using namespace OD;

struct Framebuffer_5: OD::Module {
    OD::Scene scene;
    Framebuffer* framebuffer;
    Framebuffer* tempFramebuffer;
    Mesh postProcessingMesh;
    Ref<Shader> postProcessingShader;

    Ref<Model> model;
    //CameraMovement camMove;

    Entity mainEntity;
    Entity camera;
    Entity otherEntity;

    Transform gismoTransform;

    Vector3 cubePositions[9] = {
        //Vector3( 0.0f,  0.0f,  0.0f), 
        Vector3( 2.0f,  5.0f, -15.0f), 
        Vector3(-1.5f, -2.2f, -2.5f),  
        Vector3(-3.8f, -2.0f, -12.3f),  
        Vector3( 2.4f, -0.4f, -3.5f),  
        Vector3(-1.7f,  3.0f, -7.5f),  
        Vector3( 1.3f, -2.0f, -2.5f),  
        Vector3( 1.5f,  2.0f, -2.5f), 
        Vector3( 1.5f,  0.2f, -1.5f), 
        Vector3(-4, 2.0f, -1.5f)  
    };

    Framebuffer_5() = default;
    ~Framebuffer_5() = default;

    void OnInit() override {
        LogInfo("Game Init");

        scene.RegisterSystem<PhysicsSystem>();
        scene.RegisterSystem<StandRendererSystem>();
        scene.Start();

        postProcessingMesh = Mesh::FullScreenQuad();
        postProcessingShader = Shader::CreateFromFile("res/shaders/BasicPostProcessing.glsl");

        FrameBufferSpecification framebufferSpecification = {Application::ScreenWidth(), Application::ScreenHeight()};
        //framebufferSpecification.colorAttachments.push_back({FramebufferTextureFormat::RGB});
        framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RGB}};
        framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8, true};
        
        framebuffer = new Framebuffer(framebufferSpecification);
        tempFramebuffer = new Framebuffer(framebufferSpecification);

        model = AssetManager::GetGlobal()->LoadModel("res/models/Cube.glb");
        model->SetShader(AssetManager::GetGlobal()->LoadShaderFromFile("res/shaders/model.glsl"));
        model->materials[0].SetTexture("texture1", AssetManager::GetGlobal()->LoadTexture2D("res/textures/floor.jpg", false, OD::TextureFilter::Linear, false));
        model->materials[0].SetVector3("color", Vector3(1.0f, 0.5f, 0.31f));

        Entity camera = scene.AddEntity();
        CameraComponent& cam = camera.AddComponent<CameraComponent>();
        camera.GetComponent<TransformComponent>().localPosition(Vector3(0, 2, 4));
        camera.GetComponent<TransformComponent>().localEulerAngles(Vector3(-25, 0, 0));
        //camMove.transform = &camera->transform();
        cam.farClipPlane = 50;

        mainEntity = scene.AddEntity();
        mainEntity.GetComponent<TransformComponent>().localPosition(Vector3(2, 0, 0));
        MeshRendererComponent& meshRenderer = mainEntity.AddComponent<MeshRendererComponent>();
        meshRenderer.mesh = model;

        for(int i = 0; i < 9; i++){
            Entity e = scene.AddEntity();
            MeshRendererComponent& mr = e.AddComponent<MeshRendererComponent>();
            mr.mesh = model;
            
            e.GetComponent<TransformComponent>().localPosition(cubePositions[i]);
            float angle = 20.0f * i; 
            e.GetComponent<TransformComponent>().localEulerAngles(Vector3(angle*1, angle*0.3f, angle*0.5f));
            otherEntity = e;

            scene.SetParent(mainEntity.id(), e.id());
        }
    }

    void OnUpdate(float deltaTime) override {
        //camMove.OnUpdate();

        mainEntity.GetComponent<TransformComponent>().localEulerAngles(Vector3(0, Platform::GetTime() * 40, 0));
        otherEntity.GetComponent<TransformComponent>().position(Vector3(-5, 2, -1.5f));

        scene.Update();
    }   

    void OnRender(float deltaTime) override {
        //LogInfo("Screen Width: %d Screen Height: %d", Application::ScreenWidth(), Application::ScreenHeight());

        /*
        //Pass1
        framebuffer->Bind();
        scene.Draw();
        
        //Pass2
        //Renderer::SetRenderMode(Renderer::RenderMode::WIREFRAME);
        framebuffer->Unbind();
        Renderer::Clean(1,1,1,1);
        Renderer::SetDepthTest(DepthTest::DISABLE); 

        postProcessingShader->Bind();
        postProcessingShader->SetFramebuffer("mainTex2", *framebuffer, 0, 0);
        postProcessingShader->SetFloat("option", 4);

        Renderer::DrawMeshRaw(postProcessingMesh);
        */

        /*
        //Pass1
        tempFramebuffer->Bind();
        scene.Draw();
        
        //Pass2
        framebuffer->Bind();
        Renderer::Clean(1,1,1,1);
        Renderer::SetDepthTest(DepthTest::DISABLE); 
        tempFramebuffer->BindColorAttachmentTexture(*postProcessingShader, 0);
        postProcessingShader->SetFloat("option", 3);
        Renderer::DrawMeshRaw(postProcessingMesh);

        framebuffer->Unbind();
        Renderer::Clean(1,1,1,1);
        Renderer::SetDepthTest(DepthTest::DISABLE); 
        framebuffer->BindColorAttachmentTexture(*postProcessingShader, 0);
        postProcessingShader->SetFloat("option", 2);
        Renderer::DrawMeshRaw(postProcessingMesh);
        */

        ///*
        Renderer::BeginFramebuffer(tempFramebuffer); //tempFramebuffer->Bind();
        scene.Draw();

        postProcessingShader->Bind();
        postProcessingShader->SetFloat("option", 3);
        Renderer::Blit(tempFramebuffer, framebuffer, *postProcessingShader);
        
        postProcessingShader->Bind();
        postProcessingShader->SetFloat("option", 4);
        Renderer::Blit(framebuffer, nullptr, *postProcessingShader);
        //*/
    }

    void OnGUI() override {
        /*
        ImGui::Begin("Triangle Position/Color");

        static float translation[] = {0.0, 0.0, 0.0};
        ImGui::SliderFloat3("position", translation, -5, 5);
        lightTransform.localPosition(Vector3(translation[0], translation[1], translation[2]));

        static float color[4] = { 1.0f,1.0f,1.0f,1.0f };
        ImGui::ColorEdit3("color", color);
        
        model->materials[0].SetVector3("light.diffuse",  Vector3(color[0], color[1], color[2]));
        model->materials[0].SetVector3("light.position", lightTransform.localPosition());
        
        ImGui::End();
        */
    }

    void OnResize(int width, int height) override {
        framebuffer->Resize(Application::ScreenWidth(), Application::ScreenHeight());
        framebuffer->Invalidate();
    }
};