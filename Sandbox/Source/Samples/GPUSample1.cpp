#include "OD/pch.h"
#include "GPUSample1.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/GraphicsDevice.h"
#include "OD/GPU/GPU.h"
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

const char* shaderSource = R"GLSL(
    #ifdef Vertex
    layout(location = 0) in vec3 aPos;
    layout(location = 7) in vec3 aColor;

    #ifdef Vulkan
    layout(location = 0) out vec3 vColor;
    #else
    out vec3 vColor;
    #endif

    void main(){
        gl_Position = vec4(aPos, 1.0);
        vColor = aColor;
        //vColor = vec3(1.0, 0.0, 0.0);
    }
    #endif


    #ifdef Fragment
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

BufferId vertexBuffer;
BufferId vertexBuffer2;

BufferId positionBuffer;
BufferId colorBuffer;
BufferId indexBuffer;

PipelineId pipelineSingle;
PipelineId pipelineSeparate;

//#define TestDeviceCreateBuffer

void GPUSample1::OnInit(){
    GPUDevice* gpuDevice = dynamic_cast<GPUDevice*>(Graphics::GetGraphicsDevice());
    auto& frame = Application::GetRenderFrame();

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
    vertexBuffer = gpuDevice->AllocBufferId();
    frame.resourceCommands.CreateBuffer(vertexBuffer, interleaved, sizeof(interleaved), GPUBufferUsage::Vertex, GPUBufferMemory::GPUToCPU);
    Assert(gpuDevice->GetBufferStats(vertexBuffer).type == GPUResourceStatsType::None);

    #ifdef TestDeviceCreateBuffer
    vertexBuffer2 = gpuDevice->CreateBuffer(interleaved2, sizeof(interleaved2), GPUBufferUsage::Vertex, GPUBufferMemory::GPUToCPU);
    Assert(gpuDevice->GetBufferStats(vertexBuffer2).type == GPUResourceStatsType::Created);
    #endif

    // -------------------------------------------------
    // Triangle 2: POSITION and COLOR in TWO VBOs
    // -------------------------------------------------
    positionBuffer = gpuDevice->AllocBufferId();
    frame.resourceCommands.CreateBuffer(positionBuffer, positions, sizeof(positions), GPUBufferUsage::Vertex, GPUBufferMemory::GPUOnly);

    colorBuffer = gpuDevice->AllocBufferId();
    frame.resourceCommands.CreateBuffer(colorBuffer, colors, sizeof(colors), GPUBufferUsage::Vertex, GPUBufferMemory::GPUOnly);

    // -------------------------------------------------
    // Index buffer
    // -------------------------------------------------
    indexBuffer = gpuDevice->AllocBufferId();
    frame.resourceCommands.CreateBuffer(indexBuffer, indices, sizeof(indices), GPUBufferUsage::Index, GPUBufferMemory::GPUOnly);

    // -------------------------------------------------
    // Pipeline 1: interleaved buffer
    // -------------------------------------------------
    pipelineSingle = gpuDevice->AllocPipelineId();
    GPUPipelineInfo pipelineInfo = {};
    pipelineInfo.vertexLayout.attributes[0] = {GPUVertexSemantic::Position, GPUVertexFormat::Float3, 0, 0};
    pipelineInfo.vertexLayout.attributes[1] = {GPUVertexSemantic::Color0, GPUVertexFormat::Float3, 0, 12};
    pipelineInfo.vertexLayout.attributeCount = 2;
    pipelineInfo.vertexLayout.buffers[0] = {sizeof(float) * 6, GPUVertexInputRate::Vertex};
    pipelineInfo.vertexLayout.bufferCount = 1;
    frame.resourceCommands.CreatePipeline(pipelineSingle, shaderSource, pipelineInfo);


    // -------------------------------------------------
    // Pipeline 2: separate buffers
    // -------------------------------------------------
    pipelineSeparate = gpuDevice->AllocPipelineId();
    GPUPipelineInfo separateInfo = {};
    separateInfo.vertexLayout.attributes[0] = {GPUVertexSemantic::Position, GPUVertexFormat::Float3, 0, 0};
    separateInfo.vertexLayout.attributes[1] = {GPUVertexSemantic::Color0, GPUVertexFormat::Float3, 1, 0};
    separateInfo.vertexLayout.attributeCount = 2;
    separateInfo.vertexLayout.buffers[0] = {sizeof(float) * 3, GPUVertexInputRate::Vertex};
    separateInfo.vertexLayout.buffers[1] = {sizeof(float) * 3, GPUVertexInputRate::Vertex};
    separateInfo.vertexLayout.bufferCount = 2;
    frame.resourceCommands.CreatePipeline(pipelineSeparate, shaderSource, separateInfo);
}

void GPUSample1::OnUpdate(float deltaTime){
    //SimulateHeavyWork(33);
    //if(Input::IsKeyDown(KeyCode::D)){}
}   

void GPUSample1::OnRender(float deltaTime){
    auto& frame = Application::GetRenderFrame();

    frame.renderCommands.Viewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
    frame.renderCommands.Clean(GPUClearFlags::Color | GPUClearFlags::Depth, {{0, 255, 0, 255}});

    // ================================================
    // Triangle 1
    // Position + Color in ONE VBO
    // ================================================
    frame.renderCommands.SetPipeline(pipelineSingle);
    frame.renderCommands.SetVertexBuffer(0, vertexBuffer);
    frame.renderCommands.Draw(3);

    #ifdef TestDeviceCreateBuffer
    frame.renderCommands.SetPipeline(pipelineSingle);
    frame.renderCommands.SetVertexBuffer(0, vertexBuffer2);
    frame.renderCommands.Draw(3);
    #endif

    // ================================================
    // Triangle 2
    // Position + Color in TWO VBOs
    // ================================================
    frame.renderCommands.SetPipeline(pipelineSeparate);
    frame.renderCommands.SetVertexBuffer(0, positionBuffer);
    frame.renderCommands.SetVertexBuffer(1, colorBuffer);
    frame.renderCommands.SetIndexBuffer(indexBuffer);
    frame.renderCommands.DrawIndexed(3);
}

void GPUSample1::OnExit(){}

void GPUSample1::OnGUI(){}
void GPUSample1::OnResize(int width, int height){}
