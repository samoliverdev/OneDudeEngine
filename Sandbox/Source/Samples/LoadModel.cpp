#include "OD/pch.h"
#include "LoadModel.h"
#include "Ultis/Ultis.h"
#include <OD/Core/Application.h>
#include <OD/Core/Instrumentor.h>
#include <OD/Core/ImGui.h>
#include <OD/Graphics/Model.h>
#include <OD/Graphics/Shader.h>
#include <OD/Graphics/Texture.h>
#include <OD/Graphics/InstancingBuffer.h>
#include <OD/Graphics/Graphics.h>

void LoadModelSample::OnInit(){
    LogInfo("Game Init");

    Application::Vsync(false);

    camTransform.Position(Vector3(0, 10, 70));
    camTransform.EulerAngles(Vector3(0, 0, 0));
    camMove.transform = &camTransform;

    buffer = InstancingBuffer::Create();
    buffer2 = InstancingBuffer::Create();

    model = ResourceManager::Get().LoadByPath<Model>("Sandbox/Models/cube.gltf");
    model->materials[0]->SetShader(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/Unlit.glsl"));
    auto dd = ResourceManager::Get().LoadByPath<Texture2D>("Sandbox/Textures/Rock.jpg");
    model->materials[0]->SetTexture("mainTex", dd);
    model->materials[0]->SetVector4("color", {1,1,1,1});

    for(int i = 0; i < 100/*000*/; i++){
        float posRange = 25;
        Transform t;

        float angle = 20.0f * i; 
        t.Position(Vector3(random(-posRange, posRange), random(0, posRange), random(-posRange, posRange)));
        t.EulerAngles(Vector3(random(-180, 180), random(-180, 180), random(-180, 180)));
        transforms.push_back(t.GetModelMatrix());

        transforms2.push_back({
            math::row(transforms[transforms.size()-1], 0),
            math::row(transforms[transforms.size()-1], 1),
            math::row(transforms[transforms.size()-1], 2)
        });
    }

    buffer->SetData(&transforms[0], transforms.size());
    
    buffer2->SetData(&transforms2[0], transforms2.size());

    useInstancing = false; //true;
}

void LoadModelSample::OnUpdate(float deltaTime){
    camMove.OnUpdate();
    //modelTransform.localEulerAngles(Vector3(0, Platform::GetTime() * 20, 0));
}   

void LoadModelSample::OnRender(float deltaTime){
    OD_PROFILE_SCOPE("LoadModel_2::OnRender");

    cam.SetPerspective(60, 0.1f, 1000.0f, Application::ScreenWidth(), Application::ScreenHeight());
    cam.view = math::inverse(camTransform.GetModelMatrix());

    Graphics::Begin();
   
    Graphics::SetCamera(cam);

    Graphics::BeginRenderToScreen({0, 0, 0, 0});
    Graphics::SetViewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
    Graphics::Clean(0.0f, 1.0f, 0.0f, 0.0f);

    if(useInstancing){  
        //model->materials[0]->SetEnableInstancing(true);
        model->materials[0]->DisableKeyword("SKINNED");
        if(useMatrix4x3){
            model->materials[0]->EnableKeyword("INSTANCINGMATRIX43");
        } else {
            model->materials[0]->EnableKeyword("INSTANCING");
        }

        if(useInstancingBuffer == false){
            if(useMatrix4x3){
                Graphics::DrawMeshInstancing(*model->meshs[0], *model->materials[0], &transforms2[0], transforms2.size());
            } else {
                Graphics::DrawMeshInstancing(*model->meshs[0], *model->materials[0], &transforms[0], transforms.size());
            }
        } else {
            if(useMatrix4x3){
                Graphics::DrawMeshInstancing(*model->meshs[0], *model->materials[0], *buffer2, transforms.size());
            } else {
                Graphics::DrawMeshInstancing(*model->meshs[0], *model->materials[0], *buffer, transforms.size());
            }
        }

    } else {
        model->materials[0]->DisableKeyword("SKINNED");
        model->materials[0]->DisableKeyword("INSTANCING");
        for(auto i: transforms){
            Graphics::DrawMesh(*model->meshs[0], *model->materials[0], i);
        }
    }

    //Graphics::DrawImgui(OnGUI);
    Graphics::EndRenderToScreen();
    
    Graphics::End();
}

void LoadModelSample::OnGUI(){
    //static bool show;
    //ImGui::ShowDemoWindow(&show);

    ImGui::Begin("Load Model Test");

    ImGui::Checkbox("Use Instancing", &useInstancing);
    ImGui::Checkbox("Use Instancing Buffer", &useInstancingBuffer);
    ImGui::Checkbox("Use Matrix4x3", &useMatrix4x3);
    ImGui::Spacing();
    
    ImGui::Text("DrawCalls: %d", Graphics::GetStats().drawCalls);
    //ImGui::Text("Vertices: %dk", Graphics::GetVerticesCount() / 1000);
    //ImGui::Text("Tris: %dk", Graphics::GetTrisCount() / 1000);
    
    if(Graphics::GetStats().vertices >= 1000000){
        ImGui::Text("Vertices: %.1fM", Graphics::GetStats().vertices / 1000000.0f);
    } else if(Graphics::GetStats().vertices >= 1000){
        ImGui::Text("Vertices: %.1fk", Graphics::GetStats().vertices / 1000.0f);
    } else {
        ImGui::Text("Vertices: %d", Graphics::GetStats().vertices);
    }

    if(Graphics::GetStats().tris >= 1000000){
        ImGui::Text("Tris: %.1fM", Graphics::GetStats().tris / 1000000.0f);
    } else if(Graphics::GetStats().tris >= 1000){
        ImGui::Text("Tris: %.1fk", Graphics::GetStats().tris / 1000.0f);
    } else {
        ImGui::Text("Tris: %d", Graphics::GetStats().tris);
    }
    
    ImGui::End();
}

void LoadModelSample::OnResize(int width, int height){}
void LoadModelSample::OnExit(){}