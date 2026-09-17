#include "OD/pch.h"
#include "GPUSample5.h"
#include "OD/Gfx/Gfx.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/GraphicsDevice.h"
#include "OD/Core/Application.h"
#include <stb/stb_image.h>

using namespace OD;

namespace {
    const char* shader = R"GLSL(
    #ifdef VERTEX
    layout(location = 0) in vec3 aPos;
    #ifdef Vulkan_API
    layout(location = 0) out vec3 direction;
    #else
    out vec3 direction;
    #endif
    void main(){ direction = aPos; gl_Position = vec4(aPos.xy, 0.0, 1.0); }
    #endif
    #ifdef FRAGMENT
    layout(location = 0) out vec4 FragColor;
    #ifdef Vulkan_API
    layout(set = 0, binding = 0) uniform samplerCube skybox;
    layout(location = 0) in vec3 direction;
    #else
    uniform samplerCube skybox;
    in vec3 direction;
    #endif
    void main(){ FragColor = texture(skybox, direction); }
    #endif
    )GLSL";

    Gfx::Buffer vertexBuffer = Gfx::InvalidID;
    Gfx::Pipeline pipeline = Gfx::InvalidID;
    Gfx::Cubemap cubemap = Gfx::InvalidID;
    Gfx::BindGroup bindGroup = Gfx::InvalidID;
}

void GPUSample5::OnInit(){
    auto* device = dynamic_cast<Gfx::Device*>(Graphics::GetGraphicsDevice());
    if(device == nullptr) return;
    const float vertices[] = {
        -1,-1,-1, 1,-1,-1, 1,1,-1, 1,1,-1, -1,1,-1, -1,-1,-1,
        -1,-1,1, -1,1,1, 1,1,1, 1,1,1, 1,-1,1, -1,-1,1,
        -1,1,1, -1,1,-1, 1,1,-1, 1,1,-1, 1,1,1, -1,1,1,
        -1,-1,-1, 1,-1,-1, 1,-1,1, 1,-1,1, -1,-1,1, -1,-1,-1,
        1,-1,-1, 1,1,-1, 1,1,1, 1,1,1, 1,-1,1, 1,-1,-1,
        -1,-1,-1, -1,-1,1, -1,1,1, -1,1,1, -1,1,-1, -1,-1,-1
    };
    vertexBuffer = device->CreateBuffer(sizeof(vertices), Gfx::BufferUsage::Vertex, Gfx::BufferMemory::GPUOnly);
    device->UpdatedBuffer(vertexBuffer, vertices, sizeof(vertices));

    const char* faces[] = {
        "Engine/Textures/Skybox/right.jpg", "Engine/Textures/Skybox/left.jpg",
        "Engine/Textures/Skybox/top.jpg", "Engine/Textures/Skybox/bottom.jpg",
        "Engine/Textures/Skybox/front.jpg", "Engine/Textures/Skybox/back.jpg"
    };
    std::vector<uint8_t> pixels;
    int width = 0, height = 0;
    for(const char* face : faces){
        int channels = 0;
        stbi_uc* image = stbi_load(face, &width, &height, &channels, STBI_rgb_alpha);
        if(image == nullptr || width != height){
            if(image) stbi_image_free(image);
            return;
        }
        const size_t faceSize = static_cast<size_t>(width) * height * 4;
        pixels.insert(pixels.end(), image, image + faceSize);
        stbi_image_free(image);
    }

    Gfx::CubemapInfo cubeInfo{};
    cubeInfo.width = static_cast<uint32_t>(width);
    cubeInfo.height = static_cast<uint32_t>(height);
    cubeInfo.format = Gfx::ImageFormat::R8G8B8A8_SRGB;
    cubemap = device->CreateCubemap(cubeInfo);
    device->UploadCubemap(cubemap, pixels.data(), pixels.size());

    Gfx::BindGroupLayoutInfo layoutInfo{};
    layoutInfo.entries[0] = {0, Gfx::BindingType::TextureCube, 0, false};
    layoutInfo.entriesCount = 1;
    auto layout = device->CreateBindGroupLayout(layoutInfo);
    Gfx::BindGroupInfo groupInfo{};
    groupInfo.layout = layout;
    groupInfo.entries[0].binding = 0;
    groupInfo.entries[0].cubemap = cubemap;
    groupInfo.entriesCount = 1;
    bindGroup = device->CreateBindGroup(groupInfo);

    Gfx::PipelineInfo pipelineInfo{};
    pipelineInfo.vertexLayout.attributes[0] = {Gfx::VertexSemantic::Position, Gfx::VertexFormat::Float3, 0, 0};
    pipelineInfo.vertexLayout.attributeCount = 1;
    pipelineInfo.vertexLayout.buffers[0] = {sizeof(float) * 3, Gfx::VertexInputRate::Vertex};
    pipelineInfo.vertexLayout.bufferCount = 1;
    pipelineInfo.bindGroupLayouts[0] = layout;
    pipelineInfo.bindGroupLayoutCount = 1;
    pipelineInfo.framebufferLayout = device->GetWindowFrameBufferLayout();
    pipeline = device->CreatePipeline(shader, pipelineInfo);
}

void GPUSample5::OnUpdate(float){ }
void GPUSample5::OnRender(float){
    auto* device = dynamic_cast<Gfx::Device*>(Graphics::GetGraphicsDevice());
    if(device == nullptr || pipeline == Gfx::InvalidID || bindGroup == Gfx::InvalidID) return;
    auto* cmd = device->GetCommandBuffer();
    cmd->BeginWindowFramebuffer();
    cmd->Viewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
    cmd->Clean(Gfx::ClearFlags::Color | Gfx::ClearFlags::Depth, {{0.02f, 0.02f, 0.02f, 1.0f}});
    cmd->SetPipeline(pipeline);
    cmd->SetBindGroup(0, bindGroup);
    cmd->SetVertexBuffer(0, vertexBuffer);
    cmd->Draw(36);
    cmd->EndFramebuffer();
}
void GPUSample5::OnExit(){ }
void GPUSample5::OnGUI(){ }
void GPUSample5::OnResize(int, int){ }
