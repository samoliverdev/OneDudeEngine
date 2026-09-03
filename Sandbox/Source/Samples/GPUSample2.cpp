#include "OD/pch.h"
#include "GPUSample2.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/GraphicsDevice.h"
#include "OD/Gfx/Gfx.h"
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

Gfx::Buffer vertexBuffer_2;
Gfx::Buffer indexBuffer_2;

Gfx::Pipeline pipeline_2;

Gfx::BindGroupLayout bindGroupLayout_2;
Gfx::BindGroup bindGroup_2;

void GPUSample2::OnInit(){
    Gfx::Device* gpuDevice = dynamic_cast<Gfx::Device*>(Graphics::GetGraphicsDevice());

    Gfx::BindGroupLayoutInfo bindGroupLayoutInfo = {};
    bindGroupLayoutInfo.entries[0] = {0, Gfx::BindingType::Texture2D};
    bindGroupLayoutInfo.entries[1] = {1, Gfx::BindingType::Texture2D};
    bindGroupLayoutInfo.entriesCount = 2;
    bindGroupLayout_2 = gpuDevice->CreateBindGroupLayout(bindGroupLayoutInfo);

    // -------------------------------------------------
    // Vertex buffer
    // -------------------------------------------------
    vertexBuffer_2 = gpuDevice->CreateBuffer(vertices_2, sizeof(vertices_2), Gfx::BufferUsage::Vertex, Gfx::BufferMemory::GPUOnly);

    // -------------------------------------------------
    // Index buffer
    // -------------------------------------------------
    indexBuffer_2 = gpuDevice->CreateBuffer(indices_2, sizeof(indices_2), Gfx::BufferUsage::Index, Gfx::BufferMemory::GPUOnly);

    // -------------------------------------------------
    // Pipeline
    // -------------------------------------------------
    Gfx::PipelineInfo pipelineInfo = {};
    pipelineInfo.vertexLayout.attributes[0] = { Gfx::VertexSemantic::Position, Gfx::VertexFormat::Float3, 0, 0 };
    pipelineInfo.vertexLayout.attributes[1] = { Gfx::VertexSemantic::UV0, Gfx::VertexFormat::Float2, 0, sizeof(float) * 3 };
    pipelineInfo.vertexLayout.attributeCount = 2;
    pipelineInfo.vertexLayout.buffers[0] = { sizeof(float) * 5, Gfx::VertexInputRate::Vertex };
    pipelineInfo.vertexLayout.bufferCount = 1;
    pipelineInfo.bindGroupLayouts[0] = bindGroupLayout_2;
    pipelineInfo.bindGroupLayoutCount = 1;
    pipeline_2 = gpuDevice->CreatePipeline(shaderSource_2, pipelineInfo);

    stbi_set_flip_vertically_on_load(true);  

    int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("Sandbox/Textures/image.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    Assert(pixels);

    size_t imageSize = texWidth * texHeight * 4;
    Gfx::Texture2DInfo texInfo = {};
    texInfo.width = texWidth;
    texInfo.height = texHeight;
    texInfo.format = Gfx::ImageFormat::R8G8B8A8_SRGB;
    auto texture = gpuDevice->CreateTexture2D(texInfo, pixels, imageSize);
    stbi_image_free(pixels);

	pixels = stbi_load("Sandbox/Textures/brickwall.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    Assert(pixels);

    imageSize = texWidth * texHeight * 4;
    texInfo = {};
    texInfo.width = texWidth;
    texInfo.height = texHeight;
    texInfo.format = Gfx::ImageFormat::R8G8B8A8_SRGB;
    auto texture2 = gpuDevice->CreateTexture2D(texInfo, pixels, imageSize);
    stbi_image_free(pixels);
    

    Gfx::BindGroupInfo bindGroupInfo = {};
    bindGroupInfo.layout = bindGroupLayout_2;
    bindGroupInfo.entries[0] = {};
    bindGroupInfo.entries[0].binding = 0;
    bindGroupInfo.entries[0].texture = texture;
    bindGroupInfo.entries[1] = {};
    bindGroupInfo.entries[1].binding = 1;
    bindGroupInfo.entries[1].texture = texture2;
    bindGroupInfo.entriesCount = 2;
    bindGroup_2 = gpuDevice->CreateBindGroup(bindGroupInfo);
}

void GPUSample2::OnUpdate(float deltaTime){}

void GPUSample2::OnRender(float deltaTime){
    Gfx::Device* gpuDevice = dynamic_cast<Gfx::Device*>(Graphics::GetGraphicsDevice());
    Gfx::CommandBuffer* cmd = gpuDevice->GetCommandBuffer();

    cmd->Viewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
    cmd->Clean(Gfx::ClearFlags::Color | Gfx::ClearFlags::Depth, {{0, 0, 0, 255}});

    // -------------------------------------------------
    // Draw quad
    // -------------------------------------------------
    cmd->SetPipeline(pipeline_2);
    cmd->SetVertexBuffer(0, vertexBuffer_2);
    cmd->SetIndexBuffer(indexBuffer_2);
    cmd->SetBindGroup(0, bindGroup_2);
    cmd->DrawIndexed(6);
}

void GPUSample2::OnExit(){}
void GPUSample2::OnGUI(){}
void GPUSample2::OnResize(int width, int height){}