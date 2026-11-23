#include "OD/pch.h"
#include "SSAOPostFX.h"
#include "RenderContext.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/Framebuffer.h"
#include "OD/Graphics/Material.h"
#include "OD/Graphics/Shader.h"
#include "OD/Serialization/CerealImGui.h"

namespace OD{

void SSAOFeature::OnGui() {
    cereal::ImGuiArchive gui;
    gui(*this);
    int selectedView = static_cast<int>(debugView);
    if(ImGui::Combo("Debug View", &selectedView, "Off\0Ambient Occlusion\0Bent Normals\0")){
        debugView = static_cast<DebugView>(selectedView);
    }
}

SSAOFeature::SSAOFeature(){
    enable = false;
    event = RenderPassEvent::PostProcessBeforeForward;
    #ifndef TestNewGPU_API
    aoPass = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/SSAOPostFX.glsl"));
    Assert(aoPass != nullptr);

    auto ourLerp = [](float a, float b, float f) -> float{
        return a + f * (b - a);
    };

    std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
    std::default_random_engine generator;
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
    #endif
}

void SSAOFeature::AddRenderPasses(IRenderer& renderer, RenderContext& context){
    #ifndef TestNewGPU_API
    if(context.isDeferred && compose) renderer.AddPass(this);
    #endif
}

Ref<Framebuffer> SSAOFeature::GenerateAO(RenderContext& context, bool debugBentNormals){
    #ifndef TestNewGPU_API
    auto deferred = context.GetDeferredFramebuffer();
    auto spec = deferred->Specification();
    spec.colorAttachments = {{FramebufferTextureFormat::RGBA16F}};
    spec.createDepth = false;
    
    const auto fullSpec = spec;
    int divisor = 1;
    switch(resolutionMode){
        case ResolutionMode::Half: divisor = 2; break;
        case ResolutionMode::Quarter: divisor = 4; break;
        default: break;
    }

    spec.width = std::max(1, (fullSpec.width + divisor - 1) / divisor);
    spec.height = std::max(1, (fullSpec.height + divisor - 1) / divisor);
    auto ao1 = ResourceManager::Get().Create<Framebuffer>(spec);
    const bool guided = divisor > 1 && upsampleMode == UpsampleMode::Guided;

    aoPass->SetVector4("samples", ssaoKernel.data(), 64);
    aoPass->SetTexture("texNoise", noise);
    aoPass->SetFloat("screenWidth", spec.width);
    aoPass->SetFloat("screenHeight", spec.height);
    aoPass->SetFloat("intensity", intensity);
    aoPass->SetFloat("radius", radius);
    aoPass->SetFloat("bias", bias);
    aoPass->SetInt("outputBentNormals", debugBentNormals || (publishGlobalAO && useBentNormals));
    aoPass->SetVector2("noiseScale", {spec.width / 2.0f, spec.height / 2.0f});
    aoPass->SetTexture("gNormal", deferred, 0);
    aoPass->SetTexture("gAlbedoSpec", deferred, 1);
    aoPass->SetTexture("gDepth", deferred, -1);

    Graphics::BeginFramebuffer(*ao1);
    Graphics::SetViewport(0, 0, spec.width, spec.height);
    aoPass->SetPass(0);
    Graphics::DrawFullScreenQuad(*aoPass, Matrix4Identity);
    Graphics::EndFramebuffer();

    Ref<Framebuffer> coefficients;
    if(guided){
        auto coefficientSpec = spec;
        coefficientSpec.colorAttachments = {{FramebufferTextureFormat::RGBA32F}, {FramebufferTextureFormat::RGBA32F}};
        auto localModels = ResourceManager::Get().Create<Framebuffer>(coefficientSpec);
        coefficients = ResourceManager::Get().Create<Framebuffer>(coefficientSpec);

        Graphics::BeginFramebuffer(*localModels);
        aoPass->SetPass(7); // SSAOGuidedFit
        aoPass->SetTexture("ssaoTexture", ao1, 0);
        Graphics::DrawFullScreenQuad(*aoPass, Matrix4Identity);
        Graphics::EndFramebuffer();

        Graphics::BeginFramebuffer(*coefficients);
        aoPass->SetPass(8); // SSAOGuidedAverage
        aoPass->SetTexture("guidedSlope", localModels, 0);
        aoPass->SetTexture("guidedIntercept", localModels, 1);
        Graphics::DrawFullScreenQuad(*aoPass, Matrix4Identity);
        Graphics::EndFramebuffer();
    } else {
        auto ao2 = ResourceManager::Get().Create<Framebuffer>(spec);
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
    }

    if(divisor == 1) return ao1;

    auto fullAO = ResourceManager::Get().Create<Framebuffer>(fullSpec);
    Graphics::BeginFramebuffer(*fullAO);
    Graphics::SetViewport(0, 0, fullSpec.width, fullSpec.height);
    if(guided){
        aoPass->SetPass(9); // SSAOGuidedUpsample
        aoPass->SetTexture("guidedSlope", coefficients, 0);
        aoPass->SetTexture("guidedIntercept", coefficients, 1);
    } else {
        aoPass->SetPass(6); // SSAOBilateralUpsample
        aoPass->SetInt("useSurfacePlane", upsampleMode == UpsampleMode::SurfacePlane);
        aoPass->SetTexture("ssaoTexture", ao1, 0);
    }
    Graphics::DrawFullScreenQuad(*aoPass, Matrix4Identity);
    Graphics::EndFramebuffer();

    return fullAO;
    #else
    return nullptr;
    #endif
}

void SSAOFeature::DrawDebug(const Ref<Framebuffer>& ao){
    aoPass->SetPass(5); // SSAODebug
    aoPass->SetInt("debugBentNormals", debugView == DebugView::BentNormals);
    aoPass->SetTexture("ssaoTexture", ao, 0);
    Graphics::DrawFullScreenQuad(*aoPass, Matrix4Identity);
}

void SSAOFeature::Execute(Scene& scene, RenderContext& context, RenderFrameData& data){
    if(!context.isDeferred || !compose){
        Graphics::BlitFramebuffer(data.src.get(), data.dst.get());
        return;
    }

    auto ao = publishGlobalAO ? context.GetGlobalAO() : nullptr;
    if(ao == nullptr) ao = GenerateAO(context);
    if(ao == nullptr){
        Graphics::BlitFramebuffer(data.src.get(), data.dst.get());
        return;
    }

    Graphics::BeginFramebuffer(*data.dst);
    aoPass->SetPass(3);
    aoPass->SetTexture("mainTex", data.src, 0);
    aoPass->SetTexture("ssaoTexture", ao, 0);
    Graphics::DrawFullScreenQuad(*aoPass, Matrix4Identity);
    Graphics::EndFramebuffer();
}

}
