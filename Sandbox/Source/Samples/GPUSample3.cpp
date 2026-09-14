#include "OD/pch.h"
#include "GPUSample3.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/GraphicsDevice.h"
#include "OD/Gfx/Gfx.h"
#include "OD/Gfx/GfxReflection.h"
#include "OD/Core/Application.h"
#include "OD/Core/Input.h"

using namespace OD;

// -------------------------------------------------
// Quad vertex data
// Position + UV
// -------------------------------------------------

float vertices_3[] = {
    // position          // uv
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
     0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, 0.0f,  0.0f, 1.0f
};

uint32_t indices_3[] = {
    0, 1, 2,
    2, 3, 0
};

// -------------------------------------------------
// Shader
// -------------------------------------------------

const char* shaderSource_3 = R"GLSL(
    #ifdef VERTEX
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec2 aUV;
    layout(location = 10) in mat4 aIntancingData;

    #ifdef Vulkan_API
    layout(location = 0) out vec2 vUV;
    #else
    out vec2 vUV;
    #endif

    void main(){
        mat4 model = aIntancingData;
        gl_Position = model * vec4(aPos, 1.0);
        //gl_Position.y = -gl_Position.y;
        
        vUV = aUV;
    }
    #endif

    #ifdef FRAGMENT
    #ifdef Vulkan_API
    layout(location = 0) in vec2 vUV;
    layout(location = 0) out vec4 FragColor;
    #else
    in vec2 vUV;
    out vec4 FragColor;
    #endif

    void main(){
        FragColor = vec4(vUV, 0.0, 1.0);
    }
    #endif
)GLSL";

// -------------------------------------------------
// GPU resources
// -------------------------------------------------

Gfx::Buffer vertexBuffer_3;
Gfx::Buffer indexBuffer_3;
Gfx::Buffer instancingBuffer_3;

Gfx::Pipeline pipeline_3;

Gfx::BindGroupLayout bindGroupLayout_3;

void GPUSample3::OnInit(){
    Gfx::Device* gpuDevice = dynamic_cast<Gfx::Device*>(Graphics::GetGraphicsDevice());

    Gfx::BindGroupLayoutInfo bindGroupLayoutInfo = {};
    bindGroupLayoutInfo.entriesCount = 0;
    bindGroupLayout_3 = gpuDevice->CreateBindGroupLayout(bindGroupLayoutInfo);

    // -------------------------------------------------
    // Vertex buffer
    // -------------------------------------------------
    vertexBuffer_3 = gpuDevice->CreateBuffer(sizeof(vertices_3), Gfx::BufferUsage::Vertex, Gfx::BufferMemory::GPUOnly);
    gpuDevice->UpdatedBuffer(vertexBuffer_3, vertices_3, sizeof(vertices_3));

    // -------------------------------------------------
    // Index buffer
    // -------------------------------------------------
    indexBuffer_3 = gpuDevice->CreateBuffer(sizeof(indices_3), Gfx::BufferUsage::Index, Gfx::BufferMemory::GPUOnly);
    gpuDevice->UpdatedBuffer(indexBuffer_3, indices_3, sizeof(indices_3));

    Matrix4 instancingData [2] = {math::translate(Matrix4Identity, Vector3(-0.5f, 0, 0)), math::translate(Matrix4Identity, Vector3(0.5f, 0, 0))};
    instancingBuffer_3 = gpuDevice->CreateBuffer(sizeof(Matrix4) * 2, Gfx::BufferUsage::Vertex, Gfx::BufferMemory::GPUOnly);
    gpuDevice->UpdatedBuffer(instancingBuffer_3, instancingData, sizeof(Matrix4) * 2);

    // -------------------------------------------------
    // Pipeline
    // -------------------------------------------------
    Gfx::PipelineInfo pipelineInfo = {};
    pipelineInfo.vertexLayout.attributes[0] = { Gfx::VertexSemantic::Position, Gfx::VertexFormat::Float3, 0, 0 };
    pipelineInfo.vertexLayout.attributes[1] = { Gfx::VertexSemantic::UV0, Gfx::VertexFormat::Float2, 0, sizeof(float) * 3 };

    pipelineInfo.vertexLayout.attributes[2] = { Gfx::VertexSemantic::Intancing0, Gfx::VertexFormat::Float4, 1, 0 };
    pipelineInfo.vertexLayout.attributes[3] = { Gfx::VertexSemantic::Intancing1, Gfx::VertexFormat::Float4, 1, sizeof(float) * 4 };
    pipelineInfo.vertexLayout.attributes[4] = { Gfx::VertexSemantic::Intancing2, Gfx::VertexFormat::Float4, 1, sizeof(float) * 8 };
    pipelineInfo.vertexLayout.attributes[5] = { Gfx::VertexSemantic::Intancing3, Gfx::VertexFormat::Float4, 1, sizeof(float) * 12 };

    pipelineInfo.vertexLayout.attributeCount = 6;
    pipelineInfo.vertexLayout.buffers[0] = { sizeof(float) * 5, Gfx::VertexInputRate::Vertex };
    pipelineInfo.vertexLayout.buffers[1] = { sizeof(Matrix4), Gfx::VertexInputRate::Instance };
    pipelineInfo.vertexLayout.bufferCount = 2;
    pipelineInfo.bindGroupLayouts[0] = bindGroupLayout_3;
    pipelineInfo.bindGroupLayoutCount = 1;
    pipelineInfo.framebufferLayout = gpuDevice->GetWindowFrameBufferLayout();
    pipeline_3 = gpuDevice->CreatePipeline(shaderSource_3, pipelineInfo);
}

void GPUSample3::OnUpdate(float deltaTime){}

void GPUSample3::OnRender(float deltaTime){
    Gfx::Device* gpuDevice = dynamic_cast<Gfx::Device*>(Graphics::GetGraphicsDevice());
    Gfx::CommandBuffer* cmd = gpuDevice->GetCommandBuffer();

    cmd->BeginWindowFramebuffer();
        cmd->Viewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
        cmd->Clean(Gfx::ClearFlags::Color | Gfx::ClearFlags::Depth, {{1, 0, 0, 1}});

        cmd->SetPipeline(pipeline_3);
        cmd->SetVertexBuffer(0, vertexBuffer_3);
        cmd->SetVertexBuffer(1, instancingBuffer_3);
        cmd->SetIndexBuffer(indexBuffer_3);
        cmd->DrawIndexedInstanced(6, 2);
    cmd->EndFramebuffer();
}

void GPUSample3::OnExit(){}
void GPUSample3::OnGUI(){}
void GPUSample3::OnResize(int width, int height){}
