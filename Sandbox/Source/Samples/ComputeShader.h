#pragma once
#include <OD/Core/Module.h>
#include <OD/Base.h>

namespace OD{
    class ComputeShader;
    class Framebuffer;
    class Texture2D;
    class Mesh;
    class Material;
    class UniformBuffer;
    class ComputeBuffer;
}

using namespace OD;

struct ComputeShaderSample: OD::Module {
    Ref<ComputeShader> computeShader;
    Ref<ComputeShader> computeShader2;
    Ref<Framebuffer> framebuffer;
    Ref<Texture2D> tex;
    Ref<Mesh> mesh;
    Ref<Material> mat;
    Ref<UniformBuffer> buffer;
    Ref<ComputeBuffer> computeBuffer;

    ComputeShaderSample(){ name = "ComputeShaderSample"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override; 
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};