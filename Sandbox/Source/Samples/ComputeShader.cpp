#include "OD/pch.h"
#include "ComputeShader.h"
#include <OD/Core/Application.h>
#include <OD/Graphics/ComputeShader.h>
#include <OD/Graphics/Texture.h>
#include <OD/Graphics/Framebuffer.h>
#include <OD/Graphics/Mesh.h>
#include <OD/Graphics/Shader.h>
#include <OD/Graphics/Material.h>
#include <OD/Graphics/UniformBuffer.h>
#include <OD/Graphics/ComputeBuffer.h>
#include <OD/Graphics/Graphics.h>

struct BlurParams{
    int blurRadius;
    float sigma;
    float padding[2]; // std140 alignment (16 bytes)
};

void ComputeShaderSample::OnInit(){
    computeShader = CreateRef<ComputeShader>();
    computeShader->LoadFromFile("Sandbox/ComputeShaders/Test1.compute");
    Assert(computeShader->IsValid() == true);

    computeShader2 = CreateRef<ComputeShader>();
    computeShader2->LoadFromFile("Sandbox/ComputeShaders/Test2.compute");

    computeBuffer = CreateRef<ComputeBuffer>(sizeof(float) * 1024);
    float data[1024];
    for(int i = 0; i < 1024; i++) data[i] = i;
    computeBuffer->SetData(data, sizeof(data));

    tex = Resource::CreateFromFile<Texture2D>("Sandbox/image.png");

    BlurParams params;
    params.blurRadius = 25;
    params.sigma = params.blurRadius * 0.5f;
    buffer = CreateRef<UniformBuffer>(sizeof(BlurParams));
    buffer->SetData(&params, sizeof(BlurParams));

    FrameBufferSpecification spec;
    spec.width = tex->Width();
    spec.height = tex->Height();
    spec.colorAttachments = { {FramebufferTextureFormat::RGBA16F} };
    spec.createDepth = false;
    framebuffer = CreateRef<Framebuffer>(spec);

    mesh = CreateRef<Mesh>();// Mesh::CenterQuad(true);
    mesh->vertices.push_back(OD::Vector3(0.5f, 0.5f, 0));
    mesh->vertices.push_back(OD::Vector3(0.5f, -0.5f, 0));
    mesh->vertices.push_back(OD::Vector3(-0.5f, -0.5f, 0));
    mesh->vertices.push_back(OD::Vector3(-0.5f, 0.5f, 0));
    mesh->uv.push_back(OD::Vector3(1, 1, 0));
    mesh->uv.push_back(OD::Vector3(1, 0, 0));
    mesh->uv.push_back(OD::Vector3(0, 0, 0));
    mesh->uv.push_back(OD::Vector3(0, 1, 0));
    mesh->indices.reserve(10);
    mesh->indices.push_back(0);
    mesh->indices.push_back(3);
    mesh->indices.push_back(1);
    mesh->indices.push_back(1);
    mesh->indices.push_back(3);
    mesh->indices.push_back(2);
    mesh->Submit();

    mat = ResourceManager::Get().Create<Material>(Resource::CreateFromFile<Shader>("Engine/Shaders/Unlit.glsl"));

    Graphics::BeginGPUTime();
    computeShader->SetTexture("inputTex", tex);
    computeShader->SetTexture("outputTex", framebuffer.get(), 0);
    computeShader->SetUniformBuffer("BlurParams", buffer, 2);
    computeShader->Dispatch(framebuffer->Width() / 8, framebuffer->Height() / 8, 1);
    double ms = Graphics::EndGPUTime();
    LogInfo("Compute GPU time: {} ms", ms);

    Graphics::BeginGPUTime();
    computeShader2->SetComputeBuffer("DataBuffer", computeBuffer, 0);
    computeShader2->Dispatch(16,1,1);
    double ms2 = Graphics::EndGPUTime();
    LogInfo("Compute GPU time: {} ms", ms2);

    float result[1024];
    //computeBuffer->GetData(result, sizeof(result));
    computeBuffer->GetData(result, 1024);
    Assert(result[256] == 256*2);

    mat->SetTexture("mainTex", framebuffer.get(), 0);
    //mat->SetVector4("color", {1, 0, 0, 1});
}

void ComputeShaderSample::OnUpdate(float deltaTime){

} 

void ComputeShaderSample::OnRender(float deltaTime){
    Camera cam = {Matrix4Identity, Matrix4Identity};
    Graphics::SetCamera(cam);

    Graphics::Begin();
    //Graphics::SetViewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
    Graphics::BeginRenderToScreen({0.0f, 0.0f, 1.0f, 1.0f});

    OD::Graphics::DrawMesh(*mesh, *mat, Matrix4Identity);

    OD::Graphics::EndRenderToScreen();
    OD::Graphics::End();
}

void ComputeShaderSample::OnGUI(){

}

void ComputeShaderSample::OnResize(int width, int height){

}

void ComputeShaderSample::OnExit(){

}