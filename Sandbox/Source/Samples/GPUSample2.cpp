#include "OD/pch.h"
#include "GPUSample2.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/GraphicsDevice.h"
#include "OD/GPU/GPU.h"
#include "OD/Core/Application.h"
#include "OD/Core/Input.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

using namespace OD;

// -------------------------------------------------
// Quad vertex data
// Position + UV
// -------------------------------------------------

float vertices_2[] = {
    // position          // uv
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
     0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, 0.0f,  0.0f, 1.0f
};

uint32_t indices_2[] = {
    0, 1, 2,
    2, 3, 0
};

// -------------------------------------------------
// Shader
// -------------------------------------------------

const char* shaderSource_2 = R"GLSL(
    #ifdef Vertex
    layout(location = 0) in vec3 aPos;
    layout(location = 3) in vec2 aUV;

    #ifdef Vulkan
    layout(location = 0) out vec2 vUV;
    #else
    out vec2 vUV;
    #endif

    void main(){
        gl_Position = vec4(aPos, 1.0);

        #ifdef Vulkan
            vUV = vec2(aUV.x, 1.0 - aUV.y);
        #else
            vUV = aUV;
        #endif
    }
    #endif

    #ifdef Fragment
    #ifdef Vulkan
    layout(location = 0) in vec2 vUV;
    layout(location = 0) out vec4 FragColor;
    #else
    in vec2 vUV;
    out vec4 FragColor;
    #endif

    layout(set = 0, binding = 0) uniform sampler2D tex1;
    layout(set = 0, binding = 1) uniform sampler2D tex2;

    void main(){
        FragColor = vec4(vUV, 0.0, 1.0);

        vec3 color = texture(tex1, vUV).xyz;
	    FragColor = vec4(color,1.0f);

        FragColor = mix(texture(tex1, vUV), texture(tex2, vUV), 0.2);
    }
    #endif
)GLSL";

// -------------------------------------------------
// GPU resources
// -------------------------------------------------

BufferId vertexBuffer_2;
BufferId indexBuffer_2;

PipelineId pipeline_2;

BindGroupLayoutId bindGroupLayout_2;
BindGroupId bindGroup_2;

void GPUSample2::OnInit(){
    GPUDevice* gpuDevice = dynamic_cast<GPUDevice*>(Graphics::GetGraphicsDevice());
    auto& frame = *gpuDevice->GetRenderFrame();

    GPUBindGroupLayoutInfo bindGroupLayoutInfo = {};
    bindGroupLayoutInfo.entries[0] = {0, GPUBindingType::Texture2D};
    bindGroupLayoutInfo.entries[1] = {1, GPUBindingType::Texture2D};
    bindGroupLayoutInfo.entriesCount = 2;
    bindGroupLayout_2 = gpuDevice->AllocCreateBindGroupLayoutId();
    frame.resourceCommands.CreateBindGroupLayout(bindGroupLayout_2, bindGroupLayoutInfo);

    // -------------------------------------------------
    // Vertex buffer
    // -------------------------------------------------
    vertexBuffer_2 = gpuDevice->AllocBufferId();
    frame.resourceCommands.CreateBuffer(vertexBuffer_2, vertices_2, sizeof(vertices_2), GPUBufferUsage::Vertex, GPUBufferMemory::GPUOnly);

    // -------------------------------------------------
    // Index buffer
    // -------------------------------------------------
    indexBuffer_2 = gpuDevice->AllocBufferId();
    frame.resourceCommands.CreateBuffer(indexBuffer_2, indices_2, sizeof(indices_2), GPUBufferUsage::Index, GPUBufferMemory::GPUOnly);

    // -------------------------------------------------
    // Pipeline
    // -------------------------------------------------
    pipeline_2 = gpuDevice->AllocPipelineId();
    GPUPipelineInfo pipelineInfo = {};
    pipelineInfo.vertexLayout.attributes[0] = { GPUVertexSemantic::Position, GPUVertexFormat::Float3, 0, 0 };
    pipelineInfo.vertexLayout.attributes[1] = { GPUVertexSemantic::UV0, GPUVertexFormat::Float2, 0, sizeof(float) * 3 };
    pipelineInfo.vertexLayout.attributeCount = 2;
    pipelineInfo.vertexLayout.buffers[0] = { sizeof(float) * 5, GPUVertexInputRate::Vertex };
    pipelineInfo.vertexLayout.bufferCount = 1;
    pipelineInfo.bindGroupLayouts[0] = bindGroupLayout_2;
    pipelineInfo.bindGroupLayoutCount = 1;
    frame.resourceCommands.CreatePipeline(pipeline_2, shaderSource_2, pipelineInfo);

    stbi_set_flip_vertically_on_load(true);  

    int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("Sandbox/Textures/image.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    Assert(pixels);

    auto texture = gpuDevice->AllocTexture2DId();
    size_t imageSize = texWidth * texHeight * 4;
    GPUTexture2DInfo texInfo = {};
    texInfo.width = texWidth;
    texInfo.height = texHeight;
    texInfo.format = GPUImageFormat::R8G8B8A8_SRGB;
    frame.resourceCommands.CreateTexture2D(texture, texInfo, pixels, imageSize);
    stbi_image_free(pixels);

	pixels = stbi_load("Sandbox/Textures/brickwall.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    Assert(pixels);

    auto texture2 = gpuDevice->AllocTexture2DId();
    imageSize = texWidth * texHeight * 4;
    texInfo = {};
    texInfo.width = texWidth;
    texInfo.height = texHeight;
    texInfo.format = GPUImageFormat::R8G8B8A8_SRGB;
    frame.resourceCommands.CreateTexture2D(texture2, texInfo, pixels, imageSize);
    stbi_image_free(pixels);
    

    GPUBindGroupInfo bindGroupInfo = {};
    bindGroupInfo.layout = bindGroupLayout_2;
    bindGroupInfo.entries[0] = {};
    bindGroupInfo.entries[0].binding = 0;
    bindGroupInfo.entries[0].texture = texture;
    bindGroupInfo.entries[1] = {};
    bindGroupInfo.entries[1].binding = 1;
    bindGroupInfo.entries[1].texture = texture2;
    bindGroupInfo.entriesCount = 2;
    bindGroup_2 = gpuDevice->AllocCreateBindGroupId(); 
    frame.resourceCommands.CreateBindGroup(bindGroup_2, bindGroupInfo);
}

void GPUSample2::OnUpdate(float deltaTime){}

void GPUSample2::OnRender(float deltaTime){
    GPUDevice* gpuDevice = dynamic_cast<GPUDevice*>(Graphics::GetGraphicsDevice());
    auto& frame = *gpuDevice->GetRenderFrame();

    frame.renderCommands.Viewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
    frame.renderCommands.Clean(GPUClearFlags::Color | GPUClearFlags::Depth, {{0, 0, 0, 255}});

    // -------------------------------------------------
    // Draw quad
    // -------------------------------------------------
    frame.renderCommands.SetPipeline(pipeline_2);
    frame.renderCommands.SetVertexBuffer(0, vertexBuffer_2);
    frame.renderCommands.SetIndexBuffer(indexBuffer_2);
    frame.renderCommands.SetBindGroup(0, bindGroup_2);
    frame.renderCommands.DrawIndexed(6);
}

void GPUSample2::OnExit(){}
void GPUSample2::OnGUI(){}
void GPUSample2::OnResize(int width, int height){}