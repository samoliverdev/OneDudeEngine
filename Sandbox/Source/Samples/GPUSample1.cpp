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

float vertices[] = {
    -0.5f, -0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
    0.0f,  0.5f, 0.0f
};  
const char* shaderSource = R"GLSL(
    #ifdef Vertex

    layout (location = 0) in vec3 aPos;

    void main(){
        gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    }

    #endif

    #ifdef Fragment
    out vec4 FragColor;
    void main(){
        FragColor = vec4(1.0, 0.5, 0.2, 1.0);
    }
    #endif
    )GLSL";

MeshId mesh;
PipelineId pipeline;

void GPUSample1::OnInit(){
    GPUDevice* gpuDevice = dynamic_cast<GPUDevice*>(Graphics::GetGraphicsDevice());
    auto& frame = Application::GetRenderFrame();

    mesh = gpuDevice->AllocMeshId();
    frame.commands.CreateMesh(mesh, vertices, sizeof(vertices));

    pipeline = gpuDevice->AllocPipelineId();
    frame.commands.CreatePipeline(pipeline, shaderSource, static_cast<uint32_t>(strlen(shaderSource)));
}

void GPUSample1::OnUpdate(float deltaTime){
    SimulateHeavyWork(33);

    if(Input::IsKeyDown(KeyCode::D)){
        GPUDevice* gpuDevice = dynamic_cast<GPUDevice*>(Graphics::GetGraphicsDevice());
        auto& frame = Application::GetRenderFrame();

        frame.commands.DestroyMesh(mesh);

        mesh = gpuDevice->AllocMeshId();
        frame.commands.CreateMesh(mesh, vertices, sizeof(vertices));

        LogInfo("Update Mesh: {}", mesh);
    }
}   

void GPUSample1::OnRender(float deltaTime){
    auto& frame = Application::GetRenderFrame();

    frame.commands.Viewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
    frame.commands.Clean(0, 0, 0, 255); 

    for(int i = 0; i < 90000; i++){
        frame.commands.Draw(mesh, pipeline, 3);
    }
}

void GPUSample1::OnExit(){}

void GPUSample1::OnGUI(){}
void GPUSample1::OnResize(int width, int height){}
