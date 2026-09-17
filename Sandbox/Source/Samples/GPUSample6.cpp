#include "OD/pch.h"
#include "GPUSample6.h"

#include "OD/Core/Application.h"
#include "OD/Gfx/Gfx.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/GraphicsDevice.h"

using namespace OD;

namespace {
const char* shaderSource = R"GLSL(
    #ifdef VERTEX
    layout(location = 0) in vec3 aPos;
    layout(location = 3) in vec3 aColor;
    #ifdef Vulkan_API
    layout(location = 0) out vec3 vColor;
    #else
    out vec3 vColor;
    #endif
    void main(){
        gl_Position = vec4(aPos, 1.0);
        vColor = aColor;
    }
    #endif

    #ifdef FRAGMENT
    #ifdef Vulkan_API
    layout(location = 0) in vec3 vColor;
    layout(location = 0) out vec4 FragColor;
    #else
    in vec3 vColor;
    out vec4 FragColor;
    #endif
    void main(){ FragColor = vec4(vColor, 1.0); }
    #endif
)GLSL";

Gfx::Device* device = nullptr;
Gfx::Buffer vertexBuffer = Gfx::InvalidID;
Gfx::Pipeline pipeline = Gfx::InvalidID;
Gfx::Framebuffer sourceFramebuffer = Gfx::InvalidID;
Gfx::Framebuffer destinationFramebuffer = Gfx::InvalidID;
}

void GPUSample6::OnInit(){
    device = dynamic_cast<Gfx::Device*>(Graphics::GetGraphicsDevice());
    if(device == nullptr) return;

    const float vertices[] = {
        -0.75f, -0.65f, 0.0f,  1.0f, 0.15f, 0.10f,
         0.75f, -0.65f, 0.0f,  0.10f, 1.0f, 0.20f,
         0.00f,  0.75f, 0.0f,  0.15f, 0.30f, 1.0f
    };
    vertexBuffer = device->CreateBuffer(sizeof(vertices), Gfx::BufferUsage::Vertex, Gfx::BufferMemory::GPUOnly);
    device->UpdatedBuffer(vertexBuffer, vertices, sizeof(vertices));

    Gfx::FrameBufferCreateInfo framebufferInfo{};
    framebufferInfo.width = Application::ScreenWidth();
    framebufferInfo.height = Application::ScreenHeight();
    framebufferInfo.layout.colorAttachments[0].format = Gfx::FramebufferTextureFormat::RGBA8;
    framebufferInfo.layout.colorAttachmentsCount = 1;
    sourceFramebuffer = device->CreateFramebuffer(framebufferInfo);
    destinationFramebuffer = device->CreateFramebuffer(framebufferInfo);

    Gfx::PipelineInfo pipelineInfo{};
    pipelineInfo.vertexLayout.attributes[0] = {Gfx::VertexSemantic::Position, Gfx::VertexFormat::Float3, 0, 0};
    pipelineInfo.vertexLayout.attributes[1] = {Gfx::VertexSemantic::Color0, Gfx::VertexFormat::Float3, 0, sizeof(float) * 3};
    pipelineInfo.vertexLayout.attributeCount = 2;
    pipelineInfo.vertexLayout.buffers[0] = {sizeof(float) * 6, Gfx::VertexInputRate::Vertex};
    pipelineInfo.vertexLayout.bufferCount = 1;
    pipelineInfo.framebufferLayout = framebufferInfo.layout;
    pipeline = device->CreatePipeline(shaderSource, pipelineInfo);
}

void GPUSample6::OnUpdate(float){ }

void GPUSample6::OnRender(float){
    if(device == nullptr || sourceFramebuffer == Gfx::InvalidID ||
        destinationFramebuffer == Gfx::InvalidID || pipeline == Gfx::InvalidID) return;

    auto* commands = device->GetCommandBuffer();
    commands->BeginFramebuffer(sourceFramebuffer);
    commands->Viewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
    commands->Clean(Gfx::ClearFlags::Color, {{0.03f, 0.03f, 0.03f, 1.0f}});
    commands->SetPipeline(pipeline);
    commands->SetVertexBuffer(0, vertexBuffer);
    commands->Draw(3);
    commands->EndFramebuffer();

    // Copy the first offscreen framebuffer into the second one.
    commands->BlitFramebuffer(sourceFramebuffer, destinationFramebuffer, 0);

    // Establish the window pass, then copy the second offscreen framebuffer into it.
    commands->BeginWindowFramebuffer();
    commands->EndFramebuffer();
    commands->BlitFramebuffer(destinationFramebuffer, Gfx::InvalidID, 0);
}

void GPUSample6::OnExit(){ }
void GPUSample6::OnGUI(){ }
void GPUSample6::OnResize(int, int){ }
