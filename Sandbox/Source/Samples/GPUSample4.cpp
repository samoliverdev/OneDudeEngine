#include "OD/pch.h"
#include "GPUSample4.h"
#include "OD/Gfx/Gfx.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/GraphicsDevice.h"
#include "OD/Core/Application.h"

using namespace OD;

namespace {
    const char* shader = R"GLSL(
    #ifdef VERTEX
    layout(location = 0) in vec3 aPos;
    void main(){ gl_Position = vec4(aPos, 1.0); }
    #endif
    #ifdef FRAGMENT
    #ifdef Vulkan_API
    layout(location = 0) out vec4 FragColor;
    #else
    out vec4 FragColor;
    #endif
    void main(){ FragColor = vec4(0.1, 0.7, 1.0, 1.0); }
    #endif
    )GLSL";

    Gfx::Buffer vertexBuffer = Gfx::InvalidID;
    Gfx::Pipeline arrayPipeline = Gfx::InvalidID;
    Gfx::Pipeline cubePipeline = Gfx::InvalidID;
    Gfx::Pipeline windowPipeline = Gfx::InvalidID;
    Gfx::Framebuffer arrayFramebuffer = Gfx::InvalidID;
    Gfx::Framebuffer cubeFramebuffer = Gfx::InvalidID;
}

static Gfx::Pipeline CreatePipeline(Gfx::Device* device, Gfx::FrameBufferLayout layout){
    Gfx::PipelineInfo info{};
    info.vertexLayout.attributes[0] = {Gfx::VertexSemantic::Position, Gfx::VertexFormat::Float3, 0, 0};
    info.vertexLayout.attributeCount = 1;
    info.vertexLayout.buffers[0] = {sizeof(float) * 3, Gfx::VertexInputRate::Vertex};
    info.vertexLayout.bufferCount = 1;
    info.framebufferLayout = layout;
    return device->CreatePipeline(shader, info);
}

void GPUSample4::OnInit(){
    Gfx::Device* device = dynamic_cast<Gfx::Device*>(Graphics::GetGraphicsDevice());
    const float vertices[] = {-0.8f, -0.8f, 0.0f, 0.8f, -0.8f, 0.0f, 0.0f, 0.8f, 0.0f};
    vertexBuffer = device->CreateBuffer(sizeof(vertices), Gfx::BufferUsage::Vertex, Gfx::BufferMemory::GPUOnly);
    device->UpdatedBuffer(vertexBuffer, vertices, sizeof(vertices));

    Gfx::FrameBufferCreateInfo arrayInfo{};
    arrayInfo.width = 256;
    arrayInfo.height = 256;
    arrayInfo.layout.type = Gfx::FramebufferAttachmentType::TEXTURE_2D_ARRAY;
    arrayInfo.layout.layers = 2;
    arrayInfo.layout.colorAttachments[0].format = Gfx::FramebufferTextureFormat::RGBA8;
    arrayInfo.layout.colorAttachmentsCount = 1;
    arrayFramebuffer = device->CreateFramebuffer(arrayInfo);

    Gfx::FrameBufferCreateInfo cubeInfo = arrayInfo;
    cubeInfo.layout.type = Gfx::FramebufferAttachmentType::CUBEMAP;
    cubeInfo.layout.layers = 1;
    cubeInfo.layout.colorAttachments[0].mipLevels = 2;
    cubeFramebuffer = device->CreateFramebuffer(cubeInfo);

    arrayPipeline = CreatePipeline(device, arrayInfo.layout);
    cubePipeline = CreatePipeline(device, cubeInfo.layout);
    windowPipeline = CreatePipeline(device, device->GetWindowFrameBufferLayout());
}

void GPUSample4::OnUpdate(float){}

void GPUSample4::OnRender(float){
    Gfx::Device* device = dynamic_cast<Gfx::Device*>(Graphics::GetGraphicsDevice());
    auto* cmd = device->GetCommandBuffer();

    cmd->BeginFramebuffer(arrayFramebuffer, 1, 0);
    cmd->Viewport(0, 0, 256, 256);
    cmd->Clean(Gfx::ClearFlags::Color, {{0.0f, 0.2f, 0.4f, 1.0f}});
    cmd->SetPipeline(arrayPipeline);
    cmd->SetVertexBuffer(0, vertexBuffer);
    cmd->Draw(3);
    cmd->EndFramebuffer();

    for(uint32_t face = 0; face < 6; ++face){
        cmd->BeginFramebuffer(cubeFramebuffer, face, face == 5 ? 1 : 0);
        cmd->Viewport(0, 0, face == 5 ? 128 : 256, face == 5 ? 128 : 256);
        cmd->Clean(Gfx::ClearFlags::Color, {{0.1f, 0.1f, 0.1f, 1.0f}});
        cmd->SetPipeline(cubePipeline);
        cmd->SetVertexBuffer(0, vertexBuffer);
        cmd->Draw(3);
        cmd->EndFramebuffer();
    }

    cmd->BeginWindowFramebuffer();
    cmd->Viewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
    cmd->Clean(Gfx::ClearFlags::Color | Gfx::ClearFlags::Depth, {{0.02f, 0.02f, 0.02f, 1.0f}});
    cmd->SetPipeline(windowPipeline);
    cmd->SetVertexBuffer(0, vertexBuffer);
    cmd->Draw(3);
    cmd->EndFramebuffer();
}

void GPUSample4::OnExit(){}
void GPUSample4::OnGUI(){}
void GPUSample4::OnResize(int, int){}
