#include "OD/pch.h"
#include "GPUSample9.h"
#include "GPUSampleIBLCommon.h"

#include "OD/Core/Application.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/GraphicsDevice.h"

using namespace OD;

namespace {
constexpr uint32_t BrdfSize = 512;

Gfx::Device* device = nullptr;
Gfx::Buffer quad = Gfx::InvalidID;
Gfx::Buffer brdfUniforms = Gfx::InvalidID;
Gfx::Texture2D brdfIntegration = Gfx::InvalidID;
Gfx::Framebuffer brdfFramebuffer = Gfx::InvalidID;
Gfx::Pipeline brdfPipeline = Gfx::InvalidID;
Gfx::Pipeline displayPipeline = Gfx::InvalidID;
Gfx::BindGroup brdfGroup = Gfx::InvalidID;
Gfx::BindGroup displayGroup = Gfx::InvalidID;
bool baked = false;
}

static const char* displayShader = R"GLSL(
#ifdef VERTEX
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
#ifdef Vulkan_API
layout(location = 0) out vec2 vUV;
#else
out vec2 vUV;
#endif
void main(){
    vUV = aUV;
    gl_Position = vec4(aPos, 1.0);
}
#endif

#ifdef FRAGMENT
#ifdef Vulkan_API
layout(set = 0, binding = 0) uniform sampler2D brdfMap;
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 FragColor;
#else
uniform sampler2D brdfMap;
in vec2 vUV;
out vec4 FragColor;
#endif
void main(){
    FragColor = vec4(texture(brdfMap, vUV).rg, 0.0, 1.0);
}
#endif
)GLSL";

static Gfx::FrameBufferLayout TextureLayout(){
    Gfx::FrameBufferLayout layout{};
    layout.colorAttachments[0].format = Gfx::FramebufferTextureFormat::RGBA16F;
    layout.colorAttachmentsCount = 1;
    layout.depthAttachment.format = Gfx::FramebufferDepthTextureFormat::DEPTH_COMPONENT16;
    return layout;
}

static Gfx::Buffer CreateQuad(Gfx::Device* gpu){
    const float vertices[] = {
        -1, -1, 0, 0, 0,  1, -1, 0, 1, 0,  1, 1, 0, 1, 1,
         1, 1, 0, 1, 1, -1, 1, 0, 0, 1, -1, -1, 0, 0, 0
    };
    auto buffer = gpu->CreateBuffer(sizeof(vertices), Gfx::BufferUsage::Vertex, Gfx::BufferMemory::GPUOnly);
    gpu->UpdatedBuffer(buffer, vertices, sizeof(vertices));
    return buffer;
}

static Gfx::PipelineInfo QuadPipelineInfo(Gfx::BindGroupLayout layout, Gfx::FrameBufferLayout framebuffer){
    Gfx::PipelineInfo info{};
    info.vertexLayout.attributes[0] = {Gfx::VertexSemantic::Position, Gfx::VertexFormat::Float3, 0, 0};
    info.vertexLayout.attributes[1] = {Gfx::VertexSemantic::UV0, Gfx::VertexFormat::Float2, 0, sizeof(float) * 3};
    info.vertexLayout.attributeCount = 2;
    info.vertexLayout.buffers[0] = {sizeof(float) * 5, Gfx::VertexInputRate::Vertex};
    info.vertexLayout.bufferCount = 1;
    info.bindGroupLayouts[0] = layout;
    info.bindGroupLayoutCount = 1;
    info.framebufferLayout = framebuffer;
    return info;
}

void GPUSample9::OnInit(){
    device = dynamic_cast<Gfx::Device*>(Graphics::GetGraphicsDevice());
    if(device == nullptr) return;

    quad = CreateQuad(device);
    brdfUniforms = device->CreateBuffer(sizeof(GPUSampleIBL::BrdfUniform), Gfx::BufferUsage::Uniform, Gfx::BufferMemory::GPUOnly);

    Gfx::Texture2DInfo brdfInfo{};
    brdfInfo.width = BrdfSize;
    brdfInfo.height = BrdfSize;
    brdfInfo.format = Gfx::ImageFormat::RGBA16F;
    brdfInfo.mipmap = false;
    brdfInfo.mipLevels = 1;
    brdfIntegration = device->CreateTexture2D(brdfInfo);
    brdfFramebuffer = GPUSampleIBL::CreateTextureFramebuffer(device, BrdfSize, BrdfSize);

    Gfx::BindGroupLayoutInfo brdfLayoutInfo{};
    brdfLayoutInfo.entries[0] = {0, Gfx::BindingType::UniformBuffer, sizeof(GPUSampleIBL::BrdfUniform), false};
    brdfLayoutInfo.entriesCount = 1;
    auto brdfLayout = device->CreateBindGroupLayout(brdfLayoutInfo);

    Gfx::BindGroupLayoutInfo displayLayoutInfo{};
    displayLayoutInfo.entries[0] = {0, Gfx::BindingType::Texture2D, 0, false};
    displayLayoutInfo.entriesCount = 1;
    auto displayLayout = device->CreateBindGroupLayout(displayLayoutInfo);

    auto brdfSource = GPUSampleIBL::LoadShader("Engine/Shaders/brdf.glsl");
    brdfPipeline = device->CreatePipeline(brdfSource.c_str(), QuadPipelineInfo(brdfLayout, TextureLayout()));
    displayPipeline = device->CreatePipeline(displayShader, QuadPipelineInfo(displayLayout, device->GetWindowFrameBufferLayout()));

    Gfx::BindGroupInfo brdfGroupInfo{};
    brdfGroupInfo.layout = brdfLayout;
    brdfGroupInfo.entries[0] = {0, brdfUniforms, 0, sizeof(GPUSampleIBL::BrdfUniform), false};
    brdfGroupInfo.entriesCount = 1;
    brdfGroup = device->CreateBindGroup(brdfGroupInfo);

    Gfx::BindGroupInfo displayGroupInfo{};
    displayGroupInfo.layout = displayLayout;
    displayGroupInfo.entries[0].binding = 0;
    displayGroupInfo.entries[0].texture = brdfIntegration;
    displayGroupInfo.entriesCount = 1;
    displayGroup = device->CreateBindGroup(displayGroupInfo);
}

void GPUSample9::OnUpdate(float){ }

void GPUSample9::OnRender(float){
    if(device == nullptr) return;
    auto* commands = device->GetCommandBuffer();

    if(/*!baked &&*/ brdfPipeline != Gfx::InvalidID){
        GPUSampleIBL::BrdfUniform brdfData{};
        device->UpdatedBuffer(brdfUniforms, &brdfData, sizeof(brdfData));
        commands->BeginFramebuffer(
            brdfFramebuffer, 0, 0, true,
            Gfx::ClearFlags::Color | Gfx::ClearFlags::Depth,
            {{0, 0, 0, 1}});
        commands->Viewport(0, 0, BrdfSize, BrdfSize);
        commands->SetPipeline(brdfPipeline);
        commands->SetBindGroup(0, brdfGroup);
        commands->SetVertexBuffer(0, quad);
        commands->Draw(6);
        commands->EndFramebuffer();
        commands->CopyTexture(brdfFramebuffer, 0, brdfIntegration);
        baked = true;
    }

    commands->BeginWindowFramebuffer();
    commands->Viewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
    commands->Clean(Gfx::ClearFlags::Color | Gfx::ClearFlags::Depth, {{0.02f, 0.02f, 0.02f, 1.0f}});
    if(baked && displayPipeline != Gfx::InvalidID && displayGroup != Gfx::InvalidID){
        commands->SetPipeline(displayPipeline);
        commands->SetBindGroup(0, displayGroup);
        commands->SetVertexBuffer(0, quad);
        commands->Draw(6);
    }
    commands->EndFramebuffer();
}

void GPUSample9::OnExit(){ }
void GPUSample9::OnGUI(){ }
void GPUSample9::OnResize(int, int){ }
