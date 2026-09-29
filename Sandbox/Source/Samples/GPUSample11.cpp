#include "OD/pch.h"
#include "GPUSample11.h"
#include "OD/Core/Application.h"
#include "OD/Gfx/Gfx.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/GraphicsDevice.h"

using namespace OD;

namespace {
const char* shaderSource = R"GLSL(
#ifdef VERTEX
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
#ifdef Vulkan_API
layout(location = 0) out vec2 vUV;
#else
out vec2 vUV;
#endif
void main(){ gl_Position = vec4(aPos, 1.0); vUV = aUV; }
#endif
#ifdef FRAGMENT
#ifdef Vulkan_API
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 0) uniform sampler2DArray layers;
#else
in vec2 vUV;
out vec4 FragColor;
uniform sampler2DArray layers;
#endif
void main(){
    int layer = min(int(vUV.x * 4.0), 3);
    FragColor = texture(layers, vec3(vUV, float(layer)));
}
#endif
)GLSL";

const float vertices[] = {
    -1, -1, 0, 0, 0,
     1, -1, 0, 1, 0,
     1,  1, 0, 1, 1,
    -1,  1, 0, 0, 1,
};
const uint32_t indices[] = {0, 1, 2, 2, 3, 0};
Gfx::Texture2DArray textureArray = Gfx::InvalidID;
Gfx::Buffer vertexBuffer = Gfx::InvalidID;
Gfx::Buffer indexBuffer = Gfx::InvalidID;
Gfx::Pipeline pipeline = Gfx::InvalidID;
Gfx::BindGroupLayout bindLayout = Gfx::InvalidID;
Gfx::BindGroup bindGroup = Gfx::InvalidID;
}

void GPUSample11::OnInit(){
    auto* device = dynamic_cast<Gfx::Device*>(Graphics::GetGraphicsDevice());
    Gfx::Texture2DArrayInfo textureInfo{};
    textureInfo.width = textureInfo.height = 8;
    textureInfo.arrayElements = 4;
    textureArray = device->CreateTexture2DArray(textureInfo);

    const uint8_t layerColors[4][4] = {{255, 50, 50, 255}, {50, 255, 50, 255}, {50, 50, 255, 255}, {255, 220, 50, 255}};
    for(uint32_t layer = 0; layer < 4; ++layer){
        uint8_t pixels[8 * 8 * 4];
        for(size_t pixel = 0; pixel < 64; ++pixel)
            std::memcpy(pixels + pixel * 4, layerColors[layer], 4);
        device->UploadTexture2DArray(textureArray, pixels, sizeof(pixels), static_cast<int>(layer), 0);
    }

    Gfx::BindGroupLayoutInfo layoutInfo{};
    layoutInfo.entries[0] = {0, Gfx::BindingType::Texture2DArray, 0, false};
    layoutInfo.entriesCount = 1;
    bindLayout = device->CreateBindGroupLayout(layoutInfo);
    Gfx::BindingEntry entry{};
    entry.binding = 0;
    entry.textureArray = textureArray;
    Gfx::BindGroupInfo groupInfo{};
    groupInfo.layout = bindLayout;
    groupInfo.entries = &entry;
    groupInfo.entriesCount = 1;
    bindGroup = device->CreateBindGroup(groupInfo);

    vertexBuffer = device->CreateBuffer(sizeof(vertices), Gfx::BufferUsage::Vertex, Gfx::BufferMemory::GPUOnly);
    device->UpdatedBuffer(vertexBuffer, vertices, sizeof(vertices));
    indexBuffer = device->CreateBuffer(sizeof(indices), Gfx::BufferUsage::Index, Gfx::BufferMemory::GPUOnly);
    device->UpdatedBuffer(indexBuffer, indices, sizeof(indices));

    Gfx::PipelineInfo pipelineInfo{};
    pipelineInfo.vertexLayout.attributes[0] = {Gfx::VertexSemantic::Position, Gfx::VertexFormat::Float3, 0, 0};
    pipelineInfo.vertexLayout.attributes[1] = {Gfx::VertexSemantic::UV0, Gfx::VertexFormat::Float2, 0, sizeof(float) * 3};
    pipelineInfo.vertexLayout.attributeCount = 2;
    pipelineInfo.vertexLayout.buffers[0] = {sizeof(float) * 5, Gfx::VertexInputRate::Vertex};
    pipelineInfo.vertexLayout.bufferCount = 1;
    pipelineInfo.bindGroupLayouts[0] = bindLayout;
    pipelineInfo.bindGroupLayoutCount = 1;
    pipelineInfo.framebufferLayout = device->GetWindowFrameBufferLayout();
    pipeline = device->CreatePipeline(shaderSource, pipelineInfo);
}

void GPUSample11::OnRender(float){
    auto* device = dynamic_cast<Gfx::Device*>(Graphics::GetGraphicsDevice());
    auto* commands = device->GetCommandBuffer();
    commands->BeginWindowFramebuffer();
    commands->Viewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
    commands->Clean(Gfx::ClearFlags::Color | Gfx::ClearFlags::Depth, {{0, 0, 0, 1}});
    commands->SetPipeline(pipeline);
    commands->SetBindGroup(0, bindGroup);
    commands->SetVertexBuffer(0, vertexBuffer);
    commands->SetIndexBuffer(indexBuffer);
    commands->DrawIndexed(6);
    commands->EndFramebuffer();
}

void GPUSample11::OnExit(){
    auto* device = dynamic_cast<Gfx::Device*>(Graphics::GetGraphicsDevice());
    if(textureArray != Gfx::InvalidID) device->DestroyTexture2DArray(textureArray);
    if(indexBuffer != Gfx::InvalidID) device->DestroyBuffer(indexBuffer);
    if(vertexBuffer != Gfx::InvalidID) device->DestroyBuffer(vertexBuffer);
    if(pipeline != Gfx::InvalidID) device->DestroyPipeline(pipeline);
    if(bindLayout != Gfx::InvalidID) device->DestroyBindGroupLayout(bindLayout);
}
