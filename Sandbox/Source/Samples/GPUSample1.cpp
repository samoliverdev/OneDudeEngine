#include "OD/pch.h"
#include "GPUSample1.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/GraphicsDevice.h"
#include "OD/Gfx/Gfx.h"
#include "OD/Gfx/GfxReflection.h"
#include "OD/Core/Application.h"
#include "OD/Core/Input.h"
#include <chrono>
#include <cmath>

using namespace OD;

void SimulateHeavyWork(float milliseconds){
    auto start = std::chrono::high_resolution_clock::now();

    volatile float value = 0.0f;

    while(true){
        // Some CPU-heavy floating-point work
        for(int i = 0; i < 10000; ++i){
            value += std::sin(i * 0.001f);
            value *= 1.000001f;
            value = std::sqrt(std::abs(value) + 0.0001f);
        }

        auto now = std::chrono::high_resolution_clock::now();
        float elapsed = std::chrono::duration<float, std::milli>(now - start).count();
        if(elapsed >= milliseconds) break;
    }
}

float positions[] = {
    -0.8f,  0.5f, 0.0f,
    -0.2f, -0.5f, 0.0f,
     0.4f,  0.5f, 0.0f
};

float colors[] = {
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f
};

uint32_t indices[] = {
    0, 1, 2
};

struct Data{
    glm::mat4 view;
    glm::mat4 proj;
};

const char* shaderSource = R"GLSL(
    #ifdef VERTEX
    layout(location = 0) in vec3 aPos;
    layout(location = 7) in vec3 aColor;

    #ifdef Vulkan
    layout(location = 0) out vec3 vColor;
    #else
    out vec3 vColor;
    #endif

    layout(set = 0, binding = 0) uniform CameraData{
        mat4 view;
        mat4 proj;
    };

    layout(set = 0, binding = 1) uniform ModelData{
        mat4 model;
    };

    void main(){
        gl_Position = proj * view * model * vec4(aPos, 1.0);
        vColor = aColor;
        //vColor = vec3(1.0, 0.0, 0.0);
    }
    #endif


    #ifdef FRAGMENT
    #ifdef Vulkan
    layout(location = 0) in vec3 vColor;
    layout(location = 0) out vec4 FragColor;
    #else
    in vec3 vColor;
    out vec4 FragColor;
    #endif

    void main(){
        FragColor = vec4(vColor, 1.0);
    }
    #endif
    )GLSL";

Gfx::Buffer vertexBuffer;
Gfx::Buffer vertexBuffer2;

Gfx::Buffer positionBuffer;
Gfx::Buffer colorBuffer;
Gfx::Buffer indexBuffer;

Gfx::Buffer uniformBuffer;
Gfx::Buffer uniformBuffer2;

Gfx::Pipeline pipelineSingle;
Gfx::Pipeline pipelineSeparate;

Gfx::BindGroupLayout bindGroupLayout;
Gfx::BindGroup bindGroup;

//#define TestDeviceCreateBuffer

void GPUSample1::OnInit(){
    Gfx::Device* gpuDevice = dynamic_cast<Gfx::Device*>(Graphics::GetGraphicsDevice());

    Gfx::BindGroupLayoutInfo bindGroupLayoutInfo = {};
    bindGroupLayoutInfo.entries[0] = {0, Gfx::BindingType::UniformBuffer, sizeof(Data), false};
    bindGroupLayoutInfo.entries[1] = {1, Gfx::BindingType::UniformBuffer, sizeof(glm::mat4), false};
    bindGroupLayoutInfo.entriesCount = 2;

    //bindGroupLayout = gpuDevice->CreateBindGroupLayout(bindGroupLayoutInfo);
    bindGroupLayout = gpuDevice->CreateBindGroupLayout(bindGroupLayoutInfo);

    Data camData = { glm::identity<glm::mat4>(), glm::identity<glm::mat4>()};
    glm::mat4 matrix = glm::translate(glm::identity<glm::mat4>(), glm::vec3(0.5f, 0, 0));

    uniformBuffer = gpuDevice->CreateBuffer(&camData, sizeof(camData), Gfx::BufferUsage::Uniform, Gfx::BufferMemory::GPUOnly);

    uniformBuffer2 = gpuDevice->CreateBuffer(&matrix, sizeof(glm::mat4), Gfx::BufferUsage::Uniform, Gfx::BufferMemory::GPUOnly);

    Gfx::BindGroupInfo bindGroupInfo = {};
    bindGroupInfo.layout = bindGroupLayout;
    bindGroupInfo.entries[0] = {0, uniformBuffer, 0, sizeof(Data), false};
    bindGroupInfo.entries[1] = {1, uniformBuffer2, 0, sizeof(glm::mat4), false};
    bindGroupInfo.entriesCount = 2;

    //bindGroup = gpuDevice->CreateBindGroup(bindGroupInfo);
    bindGroup = gpuDevice->CreateBindGroup(bindGroupInfo);

    // -------------------------------------------------
    // Triangle 1: POSITION + COLOR in ONE VBO
    // -------------------------------------------------
    float interleaved[] = {
        // position          // color
        -0.9f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
        -0.1f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f
    };
    float interleaved2[] = {
        // position          // color
        0.9f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
        0.1f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,
        0.5f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f
    };
    vertexBuffer = gpuDevice->CreateBuffer(interleaved, sizeof(interleaved), Gfx::BufferUsage::Vertex, Gfx::BufferMemory::GPUOnly);
    Assert(gpuDevice->GetBufferStats(vertexBuffer).type == Gfx::ResourceStatsType::None);

    // -------------------------------------------------
    // Triangle 2: POSITION and COLOR in TWO VBOs
    // -------------------------------------------------
    positionBuffer = gpuDevice->CreateBuffer(positions, sizeof(positions), Gfx::BufferUsage::Vertex, Gfx::BufferMemory::GPUOnly);

    colorBuffer = gpuDevice->CreateBuffer(colors, sizeof(colors), Gfx::BufferUsage::Vertex, Gfx::BufferMemory::GPUOnly);

    // -------------------------------------------------
    // Index buffer
    // -------------------------------------------------
    indexBuffer = gpuDevice->CreateBuffer(indices, sizeof(indices), Gfx::BufferUsage::Index, Gfx::BufferMemory::GPUOnly);

    // -------------------------------------------------
    // Pipeline 1: interleaved buffer
    // -------------------------------------------------
    Gfx::PipelineInfo pipelineInfo = {};
    pipelineInfo.vertexLayout.attributes[0] = {Gfx::VertexSemantic::Position, Gfx::VertexFormat::Float3, 0, 0};
    pipelineInfo.vertexLayout.attributes[1] = {Gfx::VertexSemantic::Color0, Gfx::VertexFormat::Float3, 0, 12};
    pipelineInfo.vertexLayout.attributeCount = 2;
    pipelineInfo.vertexLayout.buffers[0] = {sizeof(float) * 6, Gfx::VertexInputRate::Vertex};
    pipelineInfo.vertexLayout.bufferCount = 1;
    pipelineInfo.bindGroupLayouts[0] = bindGroupLayout;
    pipelineInfo.bindGroupLayoutCount = 1;
    pipelineInfo.framebufferLayout = gpuDevice->GetWindowFrameBufferLayout();
    pipelineSingle = gpuDevice->CreatePipeline(shaderSource, pipelineInfo);


    // -------------------------------------------------
    // Pipeline 2: separate buffers
    // -------------------------------------------------
    Gfx::PipelineInfo separateInfo = {};
    separateInfo.vertexLayout.attributes[0] = {Gfx::VertexSemantic::Position, Gfx::VertexFormat::Float3, 0, 0};
    separateInfo.vertexLayout.attributes[1] = {Gfx::VertexSemantic::Color0, Gfx::VertexFormat::Float3, 1, 0};
    separateInfo.vertexLayout.attributeCount = 2;
    separateInfo.vertexLayout.buffers[0] = {sizeof(float) * 3, Gfx::VertexInputRate::Vertex};
    separateInfo.vertexLayout.buffers[1] = {sizeof(float) * 3, Gfx::VertexInputRate::Vertex};
    separateInfo.vertexLayout.bufferCount = 2;
    separateInfo.bindGroupLayouts[0] = bindGroupLayout;
    separateInfo.bindGroupLayoutCount = 1;
    separateInfo.framebufferLayout = gpuDevice->GetWindowFrameBufferLayout();
    pipelineSeparate = gpuDevice->CreatePipeline(shaderSource, separateInfo);

    Gfx::ShaderReflection reflection;
    Gfx::Reflect(shaderSource, reflection);
}

void GPUSample1::OnUpdate(float deltaTime){
    //SimulateHeavyWork(16);
    //SimulateHeavyWork(33);
    //if(Input::IsKeyDown(KeyCode::D)){}
}   

void GPUSample1::OnRender(float deltaTime){
    Gfx::Device* gpuDevice = dynamic_cast<Gfx::Device*>(Graphics::GetGraphicsDevice());
    Gfx::CommandBuffer* cmd = gpuDevice->GetCommandBuffer();

    cmd->BeginWindowFramebuffer();
        cmd->Viewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
        cmd->Clean(Gfx::ClearFlags::Color | Gfx::ClearFlags::Depth, {{0, 255, 0, 255}});

        for(int i = 0; i < 1; i++){
        // ================================================
        // Triangle 1
        // Position + Color in ONE VBO
        // ================================================
        cmd->SetPipeline(pipelineSingle);
        cmd->SetBindGroup(0, bindGroup);
        cmd->SetVertexBuffer(0, vertexBuffer);
        cmd->Draw(3);

        // ================================================
        // Triangle 2
        // Position + Color in TWO VBOs
        // ================================================
        cmd->SetPipeline(pipelineSeparate);
        cmd->SetBindGroup(0, bindGroup);
        cmd->SetVertexBuffer(0, positionBuffer);
        cmd->SetVertexBuffer(1, colorBuffer);
        cmd->SetIndexBuffer(indexBuffer);
        cmd->DrawIndexed(3);
        }
    cmd->EndFramebuffer();
}

void GPUSample1::OnExit(){}

void GPUSample1::OnGUI(){}
void GPUSample1::OnResize(int width, int height){}
