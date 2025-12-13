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

void SSAOPostFX::OnGui() {
    cereal::ImGuiArchive gui;
    gui(*this);
}

SSAOPostFX::SSAOPostFX(){
    enable = false;
    aoPass = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/SSAOPostFX.glsl"));
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

void SSAOPostFX::OnRenderImage(Framebuffer* src, Framebuffer* dst, RenderContext* context){
    if(context->isDeferred == false){
        Graphics::BlitFramebuffer(src, dst);
        return;
    }

    Framebuffer* deferred = context->GetDeferredFramebuffer();

    aoPass->SetVector4("samples", ssaoKernel.data(), 64);
    aoPass->SetTexture("texNoise", noise);

    aoPass->SetFloat("intensity", intensity);
    aoPass->SetFloat("radius", radius);
    aoPass->SetFloat("bias", bias);
    aoPass->SetVector2("noiseScale", 
        //{Application::ScreenWidth() / 4.0f, Application::ScreenHeight() / 4.0f}
        {context->GetCamera().width / 2.0f, context->GetCamera().height / 2.0f}
    );

    //aoPass->SetTexture("gPosition", deferred, 0);
    aoPass->SetTexture("gNormal", deferred, 0);
    aoPass->SetTexture("gAlbedoSpec", deferred, 1);
    //aoPass->SetTexture("gEmission", deferred, 3);
    //aoPass->SetTexture("gOther", deferred, 4);
    aoPass->SetTexture("gDepth", deferred, -1);

    Graphics::BeginFramebuffer(*dst);
    aoPass->SetTexture("mainTex", src, 0);
    Graphics::DrawFullScreenQuad(*aoPass, Matrix4Identity);
    Graphics::EndFramebuffer();
}

}