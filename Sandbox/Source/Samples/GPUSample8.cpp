#include "OD/pch.h"
#include "GPUSample8.h"
#include "GPUSampleIBLCommon.h"

#include "OD/Core/Application.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/GraphicsDevice.h"

using namespace OD;

namespace {
constexpr uint32_t EnvironmentSize = 512;
constexpr uint32_t EnvironmentMipLevels = 10;
constexpr uint32_t PrefilterSize = 128;
constexpr uint32_t PrefilterMipLevels = 5;

Gfx::Device* device = nullptr;
Gfx::Buffer cube = Gfx::InvalidID;
std::array<Gfx::Buffer, 6> environmentUniformBuffers;
Gfx::Buffer uniforms = Gfx::InvalidID;
Gfx::Texture2D hdrTexture = Gfx::InvalidID;
Gfx::Cubemap environment = Gfx::InvalidID;
Gfx::Cubemap prefilter = Gfx::InvalidID;
Gfx::Framebuffer environmentFramebuffer = Gfx::InvalidID;
Gfx::Framebuffer prefilterFramebuffer = Gfx::InvalidID;
Gfx::Pipeline environmentPipeline = Gfx::InvalidID;
Gfx::Pipeline prefilterPipeline = Gfx::InvalidID;
Gfx::Pipeline displayPipeline = Gfx::InvalidID;
std::array<Gfx::BindGroup, 6> environmentGroups;
std::array<Gfx::Buffer, PrefilterMipLevels * 6> prefilterUniformBuffers;
std::array<Gfx::BindGroup, PrefilterMipLevels * 6> prefilterGroups;
Gfx::BindGroup displayGroup = Gfx::InvalidID;
bool generated = false;

struct EnvironmentUniform{
    Matrix4 projection;
    Matrix4 view;
};
}

static const char* displayShader = R"GLSL(
#ifdef VERTEX
layout(location = 0) in vec3 aPos;
#ifdef Vulkan_API
layout(set = 0, binding = 0) uniform DisplayData {
    mat4 projection;
    mat4 view;
    float roughness;
    float padding0;
    float padding1;
    float padding2;
};
layout(location = 0) out vec3 direction;
#else
layout(std140) uniform DisplayData {
    mat4 projection;
    mat4 view;
    float roughness;
    float padding0;
    float padding1;
    float padding2;
};
out vec3 direction;
#endif
void main(){
    direction = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
#endif

#ifdef FRAGMENT
#ifdef Vulkan_API
layout(set = 0, binding = 0) uniform DisplayData {
    mat4 projection;
    mat4 view;
    float roughness;
    float padding0;
    float padding1;
    float padding2;
};
layout(set = 0, binding = 1) uniform samplerCube prefilterMap;
layout(location = 0) in vec3 direction;
layout(location = 0) out vec4 FragColor;
#else
layout(std140) uniform DisplayData {
    mat4 projection;
    mat4 view;
    float roughness;
    float padding0;
    float padding1;
    float padding2;
};
uniform samplerCube prefilterMap;
in vec3 direction;
out vec4 FragColor;
#endif
void main(){
    FragColor = vec4(textureLod(prefilterMap, normalize(direction), roughness).rgb, 1.0);
}
#endif
)GLSL";

static Gfx::FrameBufferLayout EnvironmentLayout(){
    Gfx::FrameBufferLayout layout{};
    layout.type = Gfx::FramebufferAttachmentType::CUBEMAP;
    layout.colorAttachments[0].format = Gfx::FramebufferTextureFormat::RGBA16F;
    layout.colorAttachmentsCount = 1;
    layout.depthAttachment.format = Gfx::FramebufferDepthTextureFormat::DEPTH_COMPONENT16;
    return layout;
}

static Gfx::CubemapInfo CubemapInfo(uint32_t size, uint32_t mipLevels){
    Gfx::CubemapInfo info{};
    info.width = size;
    info.height = size;
    info.format = Gfx::ImageFormat::RGBA16F;
    info.mipmap = true;
    info.mipLevels = mipLevels;
    return info;
}

void GPUSample8::OnInit(){
    device = dynamic_cast<Gfx::Device*>(Graphics::GetGraphicsDevice());
    if(device == nullptr) return;

    cube = GPUSampleIBL::CreateCube(device);
    uniforms = device->CreateBuffer(sizeof(GPUSampleIBL::CaptureUniform), Gfx::BufferUsage::Uniform, Gfx::BufferMemory::GPUOnly);
    for(auto& buffer : environmentUniformBuffers)
        buffer = device->CreateBuffer(sizeof(EnvironmentUniform), Gfx::BufferUsage::Uniform, Gfx::BufferMemory::GPUOnly);
    for(auto& buffer : prefilterUniformBuffers)
        buffer = device->CreateBuffer(sizeof(GPUSampleIBL::CaptureUniform), Gfx::BufferUsage::Uniform, Gfx::BufferMemory::GPUOnly);
    hdrTexture = GPUSampleIBL::LoadHDR(device, "Sandbox/HDRIs/industrial_sunset_puresky_2k.hdr");

    auto environmentInfo = CubemapInfo(EnvironmentSize, EnvironmentMipLevels);
    auto prefilterInfo = CubemapInfo(PrefilterSize, PrefilterMipLevels);
    environment = device->CreateCubemap(environmentInfo);
    prefilter = device->CreateCubemap(prefilterInfo);
    environmentFramebuffer = GPUSampleIBL::CreateCubemapFramebuffer(device, EnvironmentSize, EnvironmentMipLevels);
    prefilterFramebuffer = GPUSampleIBL::CreateCubemapFramebuffer(device, PrefilterSize, PrefilterMipLevels);

    const auto captureLayout = GPUSampleIBL::CreateCaptureLayout(device, Gfx::BindingType::Texture2D, sizeof(EnvironmentUniform));
    const auto prefilterLayout = GPUSampleIBL::CreateCaptureLayout(device, Gfx::BindingType::TextureCube);
    const auto displayLayout = GPUSampleIBL::CreateCaptureLayout(device, Gfx::BindingType::TextureCube);

    environmentPipeline = GPUSampleIBL::CreateCapturePipeline(
        device, GPUSampleIBL::LoadShader("Engine/Shaders/EquirectangularToCubemap.glsl"),
        captureLayout, EnvironmentLayout());
    prefilterPipeline = GPUSampleIBL::CreateCapturePipeline(
        device, GPUSampleIBL::LoadShader("Engine/Shaders/Prefilter.glsl"),
        prefilterLayout, EnvironmentLayout());
    displayPipeline = GPUSampleIBL::CreateCapturePipeline(
        device, displayShader, displayLayout, device->GetWindowFrameBufferLayout());

    for(uint32_t face = 0; face < 6; ++face)
        environmentGroups[face] = GPUSampleIBL::CreateCaptureGroup(
            device, captureLayout, environmentUniformBuffers[face], hdrTexture, sizeof(EnvironmentUniform));
    for(uint32_t mip = 0; mip < PrefilterMipLevels; ++mip){
        for(uint32_t face = 0; face < 6; ++face){
            const uint32_t index = mip * 6 + face;
            prefilterGroups[index] = GPUSampleIBL::CreateCaptureCubeGroup(
                device, prefilterLayout, prefilterUniformBuffers[index], environment);
        }
    }
    displayGroup = GPUSampleIBL::CreateCaptureCubeGroup(device, displayLayout, uniforms, prefilter);
}

void GPUSample8::OnUpdate(float){ }

void GPUSample8::OnRender(float){
    if(device == nullptr) return;
    auto* commands = device->GetCommandBuffer();

    if(/*!generated &&*/ environmentPipeline != Gfx::InvalidID && prefilterPipeline != Gfx::InvalidID){
        const auto projection = GPUSampleIBL::CaptureProjection();
        const auto views = GPUSampleIBL::CaptureViews();

        for(uint32_t mip = 0; mip < EnvironmentMipLevels; ++mip){
            const uint32_t size = std::max(1u, EnvironmentSize >> mip);
            for(uint32_t face = 0; face < 6; ++face){
                EnvironmentUniform data{};
                data.projection = projection;
                data.view = views[face];
                device->UpdatedBuffer(environmentUniformBuffers[face], &data, sizeof(data));

                commands->BeginFramebuffer(environmentFramebuffer, face, mip, true);
                commands->Viewport(0, 0, size, size);
                commands->SetPipeline(environmentPipeline);
                commands->SetBindGroup(0, environmentGroups[face]);
                commands->SetVertexBuffer(0, cube);
                commands->Draw(36);
                commands->EndFramebuffer();
            }
        }
        commands->CopyTextureCubemap(environmentFramebuffer, 0, environment);

        for(uint32_t mip = 0; mip < PrefilterMipLevels; ++mip){
            const uint32_t size = std::max(1u, PrefilterSize >> mip);
            const float roughness = static_cast<float>(mip) / static_cast<float>(PrefilterMipLevels - 1);
            for(uint32_t face = 0; face < 6; ++face){
                GPUSampleIBL::CaptureUniform data{};
                data.projection = projection;
                data.view = views[face];
                data.roughness = roughness;
                const uint32_t index = mip * 6 + face;
                device->UpdatedBuffer(prefilterUniformBuffers[index], &data, sizeof(data));

                commands->BeginFramebuffer(prefilterFramebuffer, face, mip, true);
                commands->Viewport(0, 0, size, size);
                commands->SetPipeline(prefilterPipeline);
                commands->SetBindGroup(0, prefilterGroups[index]);
                commands->SetVertexBuffer(0, cube);
                commands->Draw(36);
                commands->EndFramebuffer();
            }
        }
        commands->CopyTextureCubemap(prefilterFramebuffer, 0, prefilter);
        generated = true;
    }

    commands->BeginWindowFramebuffer();
    commands->Viewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
    commands->Clean(Gfx::ClearFlags::Color | Gfx::ClearFlags::Depth, {{0.02f, 0.02f, 0.02f, 1.0f}});
    if(generated && displayPipeline != Gfx::InvalidID){
        GPUSampleIBL::CaptureUniform displayData{};
        displayData.projection = static_cast<Matrix4>(glm::perspective(
            glm::radians(60.0f),
            static_cast<float>(Application::ScreenWidth()) / static_cast<float>(Application::ScreenHeight()),
            0.1f, 100.0f));
        displayData.view = static_cast<Matrix4>(glm::lookAt(
            Vector3(0.0f, 0.0f, 3.0f), Vector3(0.0f), Vector3(0.0f, 1.0f, 0.0f)));
        displayData.roughness = 2.0f;
        device->UpdatedBuffer(uniforms, &displayData, sizeof(displayData));

        commands->SetPipeline(displayPipeline);
        commands->SetBindGroup(0, displayGroup);
        commands->SetVertexBuffer(0, cube);
        commands->Draw(36);
    }
    commands->EndFramebuffer();
}

void GPUSample8::OnExit(){ }
void GPUSample8::OnGUI(){ }
void GPUSample8::OnResize(int, int){ }
