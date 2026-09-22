#include "OD/pch.h"
#include "GPUSample10.h"

#include "OD/Core/Application.h"
#include "OD/Gfx/Gfx.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/GraphicsDevice.h"

using namespace OD;

namespace {

struct CameraData {
    glm::mat4 view;
    glm::mat4 proj;
};

const float quadVertices[] = {
    // position          // color
    -0.28f, -0.22f, 0.0f,  1.0f, 0.2f, 0.2f,
     0.28f, -0.22f, 0.0f,  0.2f, 1.0f, 0.2f,
     0.28f,  0.22f, 0.0f,  0.2f, 0.2f, 1.0f,
    -0.28f,  0.22f, 0.0f,  1.0f, 1.0f, 0.2f,
};

const uint32_t quadIndices[] = {
    0, 1, 2,
    2, 3, 0,
};

const float triangleVertices[] = {
    // position          // color
     0.0f,  0.30f, 0.0f,  1.0f, 0.2f, 0.2f,
    -0.30f, -0.24f, 0.0f,  0.2f, 1.0f, 0.2f,
     0.30f, -0.24f, 0.0f,  0.2f, 0.2f, 1.0f,
};

const uint32_t triangleIndices[] = { 0, 1, 2 };

const char* shaderSource = R"GLSL(
    #ifdef VERTEX
    layout(location = 0) in vec3 aPos;
    layout(location = 3) in vec3 aColor;

    #ifdef Vulkan_API
    layout(location = 0) out vec3 vColor;
    layout(set = 0, binding = 0) uniform CameraData {
        mat4 view;
        mat4 proj;
    };
    layout(set = 0, binding = 1) uniform ModelData {
        mat4 model;
    };
    #else
    out vec3 vColor;
    layout(std140) uniform CameraData {
        mat4 view;
        mat4 proj;
    };
    layout(std140) uniform ModelData {
        mat4 model;
    };
    #endif

    void main() {
        gl_Position = model * vec4(aPos, 1.0);
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

    void main() {
        FragColor = vec4(vColor, 1.0);
    }
    #endif
)GLSL";

constexpr uint32_t vertexStride = sizeof(float) * 6;

Gfx::Buffer cameraBuffer;
Gfx::Buffer modelBuffers[4];
Gfx::Buffer quadVertexBuffer;
Gfx::Buffer quadIndexBuffer;
Gfx::Buffer triangleVertexBuffer;
Gfx::Buffer triangleIndexBuffer;
Gfx::Pipeline pipeline;
Gfx::BindGroupLayout bindGroupLayout;
Gfx::BindGroup bindGroups[4];

} // namespace

void GPUSample10::OnInit() {
    Gfx::Device* gpuDevice = dynamic_cast<Gfx::Device*>(Graphics::GetGraphicsDevice());

    Gfx::BindGroupLayoutInfo bindGroupLayoutInfo = {};
    bindGroupLayoutInfo.entries[0] = {0, Gfx::BindingType::UniformBuffer, sizeof(CameraData), false};
    bindGroupLayoutInfo.entries[1] = {1, Gfx::BindingType::UniformBuffer, sizeof(glm::mat4), false};
    bindGroupLayoutInfo.entriesCount = 2;
    bindGroupLayout = gpuDevice->CreateBindGroupLayout(bindGroupLayoutInfo);

    const CameraData cameraData = {
        glm::identity<glm::mat4>(),
        glm::identity<glm::mat4>(),
    };
    cameraBuffer = gpuDevice->CreateBuffer(sizeof(cameraData), Gfx::BufferUsage::Uniform, Gfx::BufferMemory::GPUOnly);
    gpuDevice->UpdatedBuffer(cameraBuffer, &cameraData, sizeof(cameraData));

    const glm::vec3 positions[] = {
        {-0.5f,  0.5f, 0.0f},
        { 0.5f,  0.5f, 0.0f},
        {-0.5f, -0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f},
    };
    for(int i = 0; i < 4; ++i){
        const glm::mat4 model = glm::translate(glm::identity<glm::mat4>(), positions[i]);
        modelBuffers[i] = gpuDevice->CreateBuffer(sizeof(model), Gfx::BufferUsage::Uniform, Gfx::BufferMemory::GPUOnly);
        gpuDevice->UpdatedBuffer(modelBuffers[i], &model, sizeof(model));

        Gfx::BindingEntry bindGroupEntries[2];

        Gfx::BindGroupInfo bindGroupInfo = {};
        bindGroupInfo.layout = bindGroupLayout;
        //bindGroupInfo.entries[0] = {0, cameraBuffer, 0, sizeof(cameraData), false};
        //bindGroupInfo.entries[1] = {1, modelBuffers[i], 0, sizeof(model), false};
        bindGroupEntries[0].binding = 0;
        bindGroupEntries[0].buffer = cameraBuffer;
        bindGroupEntries[0].size = sizeof(cameraData);
        bindGroupEntries[0].dynamicOffset = false;
        bindGroupEntries[1].binding = 1;
        bindGroupEntries[1].buffer = modelBuffers[i];
        bindGroupEntries[1].size = sizeof(model);
        bindGroupEntries[1].dynamicOffset = false;
        bindGroupInfo.entriesCount = 2;
        bindGroupInfo.entries = bindGroupEntries;
        bindGroups[i] = gpuDevice->CreateBindGroup(bindGroupInfo);
    }

    quadVertexBuffer = gpuDevice->CreateBuffer(sizeof(quadVertices), Gfx::BufferUsage::Vertex, Gfx::BufferMemory::GPUOnly);
    gpuDevice->UpdatedBuffer(quadVertexBuffer, quadVertices, sizeof(quadVertices));
    quadIndexBuffer = gpuDevice->CreateBuffer(sizeof(quadIndices), Gfx::BufferUsage::Index, Gfx::BufferMemory::GPUOnly);
    gpuDevice->UpdatedBuffer(quadIndexBuffer, quadIndices, sizeof(quadIndices));

    triangleVertexBuffer = gpuDevice->CreateBuffer(sizeof(triangleVertices), Gfx::BufferUsage::Vertex, Gfx::BufferMemory::GPUOnly);
    gpuDevice->UpdatedBuffer(triangleVertexBuffer, triangleVertices, sizeof(triangleVertices));
    triangleIndexBuffer = gpuDevice->CreateBuffer(sizeof(triangleIndices), Gfx::BufferUsage::Index, Gfx::BufferMemory::GPUOnly);
    gpuDevice->UpdatedBuffer(triangleIndexBuffer, triangleIndices, sizeof(triangleIndices));

    Gfx::PipelineInfo pipelineInfo = {};
    pipelineInfo.vertexLayout.attributes[0] = {Gfx::VertexSemantic::Position, Gfx::VertexFormat::Float3, 0, 0};
    pipelineInfo.vertexLayout.attributes[1] = {Gfx::VertexSemantic::Color0, Gfx::VertexFormat::Float3, 0, sizeof(float) * 3};
    pipelineInfo.vertexLayout.attributeCount = 2;
    pipelineInfo.vertexLayout.buffers[0] = {vertexStride, Gfx::VertexInputRate::Vertex};
    pipelineInfo.vertexLayout.bufferCount = 1;
    pipelineInfo.bindGroupLayouts[0] = bindGroupLayout;
    pipelineInfo.bindGroupLayoutCount = 1;
    pipelineInfo.framebufferLayout = gpuDevice->GetWindowFrameBufferLayout();
    pipeline = gpuDevice->CreatePipeline(shaderSource, pipelineInfo);
}

void GPUSample10::OnUpdate(float deltaTime) {}

void GPUSample10::OnRender(float deltaTime) {
    Gfx::Device* gpuDevice = dynamic_cast<Gfx::Device*>(Graphics::GetGraphicsDevice());
    Gfx::CommandBuffer* cmd = gpuDevice->GetCommandBuffer();

    cmd->BeginWindowFramebuffer();
    cmd->Viewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
    cmd->Clean(Gfx::ClearFlags::Color | Gfx::ClearFlags::Depth, {{0, 255, 0, 255}});
    cmd->SetPipeline(pipeline);

    /*
    cmd->SetBindGroup(0, bindGroups[0]);
    cmd->SetVertexBuffer(0, quadVertexBuffer);
    cmd->SetIndexBuffer(quadIndexBuffer);
    cmd->DrawIndexed(6);

    cmd->SetBindGroup(0, bindGroups[1]);
    cmd->SetVertexBuffer(0, triangleVertexBuffer);
    cmd->SetIndexBuffer(triangleIndexBuffer);
    cmd->DrawIndexed(3);

    cmd->SetBindGroup(0, bindGroups[2]);
    cmd->SetVertexBuffer(0, quadVertexBuffer);
    cmd->SetIndexBuffer(quadIndexBuffer);
    cmd->DrawIndexed(6);

    cmd->SetBindGroup(0, bindGroups[3]);
    cmd->SetVertexBuffer(0, triangleVertexBuffer);
    cmd->SetIndexBuffer(triangleIndexBuffer);
    cmd->DrawIndexed(3);
    */

    // Bind the quad geometry once and draw it twice with two model bind groups.
    cmd->SetVertexBuffer(0, quadVertexBuffer);
    cmd->SetIndexBuffer(quadIndexBuffer);
    
    cmd->SetBindGroup(0, bindGroups[0]);
    cmd->DrawIndexed(6);
    cmd->SetBindGroup(0, bindGroups[2]);
    cmd->DrawIndexed(6);

    // Bind the triangle geometry once and draw it twice with two model bind groups.
    cmd->SetVertexBuffer(0, triangleVertexBuffer);
    cmd->SetIndexBuffer(triangleIndexBuffer);
   
    cmd->SetBindGroup(0, bindGroups[1]);
    cmd->DrawIndexed(3);
    cmd->SetBindGroup(0, bindGroups[3]);
    cmd->DrawIndexed(3);

    cmd->EndFramebuffer();
}

void GPUSample10::OnExit() {}
void GPUSample10::OnGUI() {}
void GPUSample10::OnResize(int width, int height) {}
