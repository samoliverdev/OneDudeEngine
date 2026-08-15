#include "OD/pch.h"
#include "SSAOPostFX.h"
#include "RenderContext.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/Framebuffer.h"
#include "OD/Graphics/Material.h"
#include "OD/Graphics/Shader.h"
#include "OD/Core/Application.h"
#include "OD/Serialization/CerealImGui.h"

namespace OD{

void SSAOFeature::OnGui() {
    cereal::ImGuiArchive gui;
    gui(*this);
}

SSAOFeature::SSAOFeature(){
    enable = false;
    event = RenderPassEvent::PostProcessBeforeForward;
    aoPass = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/SSAOPostFX.glsl"));
    Assert(aoPass != nullptr);

    auto ourLerp = [](float a, float b, float f) -> float{
        return a + f * (b - a);
    };

    std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
    std::default_random_engine generator;
    ssaoKernel;
    for (unsigned int i = 0; i < 64; ++i){
        glm::vec3 sample(
            randomFloats(generator) * 2.0 - 1.0, 
            randomFloats(generator) * 2.0 - 1.0, 
            randomFloats(generator)
        );
        sample  = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = float(i) / 64.0f;

        // scale samples s.t. they're more aligned to center of kernel
        scale = ourLerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(Vector4(sample, 1)); 
    }

    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++){
        glm::vec3 noise(
            randomFloats(generator) * 2.0 - 1.0, 
            randomFloats(generator) * 2.0 - 1.0, 
            0.0f
        ); 
        ssaoNoise.push_back(noise);
    } 
    noise = Texture2D::CreateFromRaw(
        ssaoNoise.data(), 4, 4, TextureDataType::Float, 
        {TextureFilter::Nearest, TextureWrapping::Repeat, false, TextureFormat::RGB32F}
    );
    Assert(noise != nullptr);
}

void SSAOFeature::AddRenderPasses(IRenderer& renderer, RenderContext& context){
    renderer.AddPass(this);
}

void SSAOFeature::Execute(Scene& scene, RenderContext& context, RenderFrameData& data){
    if(context.isDeferred == false){
        Graphics::BlitFramebuffer(data.src.get(), data.dst.get());
        return;
    }
    auto spec = data.src->Specification();
    spec.colorAttachments[0].colorFormat = FramebufferTextureFormat::RGBA16F;
    spec.createDepth = false;
    
    auto ao1 = ResourceManager::Get().Create<Framebuffer>(spec);
    auto ao2 = ResourceManager::Get().Create<Framebuffer>(spec);

    Ref<Framebuffer> deferred = context.GetDeferredFramebuffer();

    aoPass->SetVector4("samples", ssaoKernel.data(), 64);
    aoPass->SetTexture("texNoise", noise);
    aoPass->SetFloat("screenWidth", context.GetCamera().width);
    aoPass->SetFloat("screenHeight", context.GetCamera().height);
    aoPass->SetFloat("intensity", intensity);
    aoPass->SetFloat("radius", radius);
    aoPass->SetFloat("bias", bias);
    aoPass->SetVector2("noiseScale", 
        //{Application::ScreenWidth() / 4.0f, Application::ScreenHeight() / 4.0f}
        {context.GetCamera().width / 2.0f, context.GetCamera().height / 2.0f}
    );
    aoPass->SetTexture("gNormal", deferred, 0);
    aoPass->SetTexture("gAlbedoSpec", deferred, 1);
    aoPass->SetTexture("gDepth", deferred, -1);

    Graphics::BeginFramebuffer(*ao1);
    aoPass->SetPass(0);
    aoPass->SetTexture("mainTex", data.src, 0);
    Graphics::DrawFullScreenQuad(*aoPass, Matrix4Identity);
    Graphics::EndFramebuffer();

    Graphics::BeginFramebuffer(*ao2);
    aoPass->SetPass(1);
    aoPass->SetTexture("ssaoTexture", ao1, 0);
    Graphics::DrawFullScreenQuad(*aoPass, Matrix4Identity);
    Graphics::EndFramebuffer();

    Graphics::BeginFramebuffer(*ao1);
    aoPass->SetPass(2);
    aoPass->SetTexture("ssaoTexture", ao2, 0);
    Graphics::DrawFullScreenQuad(*aoPass, Matrix4Identity);
    Graphics::EndFramebuffer();

    Graphics::BeginFramebuffer(*data.dst);
    aoPass->SetPass(3);
    aoPass->SetTexture("mainTex", data.src, 0);
    aoPass->SetTexture("ssaoTexture", ao1, 0);
    Graphics::DrawFullScreenQuad(*aoPass, Matrix4Identity);
    Graphics::EndFramebuffer();

    /*if(context.isDeferred == false){
        Graphics::BlitFramebuffer(data.src, data.dst);
        return;
    }

    Framebuffer* deferred = context.GetDeferredFramebuffer();

    aoPass->SetVector4("samples", ssaoKernel.data(), 64);
    aoPass->SetTexture("texNoise", noise);

    aoPass->SetFloat("screenWidth", context.GetCamera().width);
    aoPass->SetFloat("screenHeight", context.GetCamera().height);

    aoPass->SetFloat("intensity", intensity);
    aoPass->SetFloat("radius", radius);
    aoPass->SetFloat("bias", bias);
    aoPass->SetVector2("noiseScale", 
        //{Application::ScreenWidth() / 4.0f, Application::ScreenHeight() / 4.0f}
        {context.GetCamera().width / 2.0f, context.GetCamera().height / 2.0f}
    );

    aoPass->SetTexture("gNormal", deferred, 0);
    aoPass->SetTexture("gAlbedoSpec", deferred, 1);
    aoPass->SetTexture("gDepth", deferred, -1);

    Graphics::BeginFramebuffer(*data.dst);
    aoPass->SetTexture("mainTex", data.src, 0);
    Graphics::DrawFullScreenQuad(*aoPass, Matrix4Identity);
    Graphics::EndFramebuffer();*/
}

}