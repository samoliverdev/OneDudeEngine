#include "OD/pch.h"
#include "SSGIPostFX.h"
#include "RenderContext.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/Framebuffer.h"
#include "OD/Graphics/Material.h"
#include "OD/Graphics/Shader.h"
#include "OD/Core/Application.h"
#include "OD/Serialization/CerealImGui.h"

namespace OD{

void SSGIFeature::OnGui(){
    cereal::ImGuiArchive gui;
    gui(*this);
}

SSGIFeature::SSGIFeature(){
    enable = false;
    event = RenderPassEvent::PostProcess;
    
    blitPass = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/Blit.glsl"));
    giPass = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/SSGIPostFX3.glsl"));
    giBlurPass = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/SSGIBlurPostFX3.glsl"));
    giComposePass = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/SSGIComposePostFX.glsl"));
    giUpsamplePass = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/SSGIUpsample.glsl"));
    blueNoise = ResourceManager::Get().LoadByPath<Texture2D>("Engine/Textures/LDR_RG01_47.png");
}

SSGIFeature::~SSGIFeature(){
    if(lastIndirect != nullptr) delete lastIndirect;
}

void SSGIFeature::AddRenderPasses(IRenderer& renderer, RenderContext& context){
    renderer.AddPass(this);
}

void SSGIFeature::Execute(RenderContext& context, RenderFrameData& data){
    if(context.isDeferred == false){
        Graphics::BlitFramebuffer(data.src, data.dst);
        return;
    }

    auto Blit = [](Framebuffer* _src, Framebuffer* _dst, Ref<Material> blitMat, int pass = 0){
        Graphics::BeginFramebuffer(*_dst);
        Graphics::SetViewport(0, 0, _dst->Specification().width, _dst->Specification().height);
        blitMat->SetPass(pass);
        blitMat->SetTexture("mainTex", _src, 0);
        Graphics::DrawFullScreenQuad(*blitMat, Matrix4Identity);
        Graphics::EndFramebuffer();
    };

    ///*
    Framebuffer* deferred = context.GetDeferredFramebuffer();
    auto spec = data.src->Specification();
    spec.colorAttachments[0].colorFormat = FramebufferTextureFormat::RGBA16F;

    auto halfSpec = spec;
    halfSpec.width /= 2;
    halfSpec.height /= 2;

    auto gi = new Framebuffer(halfSpec);

    std::array<Framebuffer*, 16> textures;
    std::vector<Framebuffer*> releaseTemporary;

    Camera cam = context.GetCamera();
    float Deg2Rad = (math::pi<float>() * 2.0f) / 360.0f;
    //float halfProjScale = spec.height / ( math::tan(cam.fov * Deg2Rad * 0.5 ) * 2 ) * 0.5;
    float halfProjScale = spec.height / (2.0f *  math::tan(math::degrees(cam.fov) * 0.5f * Deg2Rad));

    Graphics::BeginFramebuffer(*gi);
    Graphics::SetViewport(0, 0, halfSpec.width, halfSpec.height);
    giPass->SetTexture("mainTex", data.src, 0); //giPass->SetTexture("mainTex", lighting, 0); //giPass->SetTexture("mainTex", src, 0);
    giPass->SetTexture("gNormal", deferred, 0); //giPass->SetTexture("gNormal", deferred, 1);
    giPass->SetTexture("gDepth", deferred, -1);
    giPass->SetTexture("gAlbedoSpec", deferred, 1);
    giPass->SetTexture("lastIndirect", lastIndirect, 0); //giPass->SetTexture("gNormal", deferred, 1);
    giPass->SetTexture("noise", blueNoise);
    giPass->SetFloat("sampleCount", sampleCount);
    giPass->SetFloat("sampleRadius", sampleRadius);
    giPass->SetFloat("sliceCount", sliceCount);
    giPass->SetFloat("hitThickness", hitThickness);
    giPass->SetFloat("giIntensity", giIntensity);
    giPass->SetFloat("aoIntensity", aoIntensity);
    giPass->SetVector2("screenSize", {spec.width, spec.height});
    giPass->SetFloat("useScreenSpaceSampling", useScreenSpaceSampling ? 1.0f : 0.0f);
    giPass->SetFloat("temporalRotation", 1.0f);
    giPass->SetFloat("backfaceLighting", backfaceLighting);
    giPass->SetFloat("cameraNear", cam.nearClip);
    giPass->SetFloat("cameraFar", cam.farClip);
    giPass->SetFloat("halfProjScale", halfProjScale);
    giPass->SetFloat("_HalfProjScale", halfProjScale);
    giPass->SetVector4("_Resolution", {spec.width, spec.height, 1.0f / spec.width, 1.0f / spec.height});
    Graphics::DrawFullScreenQuad(*giPass, Matrix4Identity);
    Graphics::EndFramebuffer();

    giBlurPass->SetTexture("gDepth", deferred, -1);
    giBlurPass->SetTexture("gNormal", deferred, 0);
    giBlurPass->SetFloat("cameraNear", cam.nearClip);
    giBlurPass->SetFloat("cameraFar", cam.farClip);
    giBlurPass->SetFloat("depthPhi", 100.0f*1);
    giBlurPass->SetFloat("normalPhi", 32.0f*1);

    if(denoiseMaxIterations > 0){
        //Blur Pass
        Framebuffer* currentDestination = textures[0] = new Framebuffer(spec);
        

        giBlurPass->SetVector2("giTexelSize", {1.0f / spec.width, 1.0f / spec.height});
        Blit(gi, currentDestination, giBlurPass, 0);

        Framebuffer* currentSource = currentDestination;

        int i = 1;
        for(; i < denoiseMaxIterations; i++){
            spec.width /= 2;
            spec.height /= 2;
            if(spec.height < 2){
                break;
            }

            currentDestination = textures[i] = new Framebuffer(spec);

            giBlurPass->SetVector2("giTexelSize", {1.0f / spec.width, 1.0f / spec.height});
            Blit(currentSource, currentDestination, giBlurPass, 0);
            //releaseTemporary.push_back(currentSource);
            
            currentSource = currentDestination;
        }
        for(i -= 2; i >= 0; i--){
            currentDestination = textures[i];
            textures[i] = nullptr;

            giBlurPass->SetVector2("giTexelSize", {1.0f / currentDestination->Specification().width, 1.0f / currentDestination->Specification().height});
            Blit(currentSource, currentDestination, giBlurPass, 1);
            releaseTemporary.push_back(currentSource);
            
            currentSource = currentDestination;
        }

        giBlurPass->SetVector2("giTexelSize", {1.0f / deferred->Specification().width, 1.0f / deferred->Specification().height});
        Blit(currentSource, gi, giBlurPass, 1);
        releaseTemporary.push_back(currentSource);
        //
    }

    giComposePass->SetPass(debug ? 1 : 0);
    giComposePass->SetVector2("giSize", {(float)deferred->Specification().width, (float)deferred->Specification().height});
    giComposePass->SetVector2("screenSize", {(float)deferred->Specification().width, (float)deferred->Specification().height});
    Graphics::BeginFramebuffer(*data.dst);
    Graphics::SetViewport(0, 0, deferred->Specification().width, deferred->Specification().height);
    giComposePass->SetTexture("mainTex", data.src, 0);
    giComposePass->SetTexture("gAlbedoSpec", deferred, 1);
    giComposePass->SetTexture("giAO", gi, 0);
    Graphics::DrawFullScreenQuad(*giComposePass, Matrix4Identity);
    Graphics::EndFramebuffer();


    for(Framebuffer* cur: releaseTemporary) delete cur;

    delete gi;
    //*/

    /*
    Framebuffer* deferred = context->GetDeferredFramebuffer();
    auto spec = src->Specification();

    if(lastIndirect == nullptr) lastIndirect = new Framebuffer(spec);
    lastIndirect->Resize(spec.width, spec.height);

    auto halfSpec = spec;
    halfSpec.width /= 2; //2;
    halfSpec.height /= 2; //2;

    auto normal = new Framebuffer(spec);
    auto lighting = new Framebuffer(spec);
    auto ping = new Framebuffer(halfSpec);
    auto pong = new Framebuffer(halfSpec);

    Graphics::BeginFramebuffer(*normal);
    Graphics::SetViewport(0, 0, spec.width, spec.height);
    blitPass->SetTexture("mainTex", deferred, 0);
    Graphics::DrawFullScreenQuad(*blitPass, Matrix4Identity);
    Graphics::EndFramebuffer();

    Graphics::BeginFramebuffer(*lighting);
    Graphics::SetViewport(0, 0, spec.width, spec.height);
    blitPass->SetTexture("mainTex", src, 0);
    Graphics::DrawFullScreenQuad(*blitPass, Matrix4Identity);
    Graphics::EndFramebuffer();

    Graphics::BeginFramebuffer(*ping);
    Graphics::SetViewport(0, 0, spec.width, spec.height);
    giPass->SetTexture("mainTex", lighting, 0); //giPass->SetTexture("mainTex", lighting, 0); //giPass->SetTexture("mainTex", src, 0);
    giPass->SetTexture("gNormal", normal, 0); //giPass->SetTexture("gNormal", deferred, 1);
    giPass->SetTexture("gDepth", deferred, -1);
    giPass->SetTexture("gAlbedoSpec", deferred, 1);
    giPass->SetTexture("lastIndirect", lastIndirect, 0); //giPass->SetTexture("gNormal", deferred, 1);
    giPass->SetTexture("noise", blueNoise);
    giPass->SetFloat("sampleCount", sampleCount);
    giPass->SetFloat("sampleRadius", sampleRadius);
    giPass->SetFloat("sliceCount", sliceCount);
    giPass->SetFloat("hitThickness", hitThickness);
    giPass->SetFloat("giIntensity", giIntensity);
    giPass->SetFloat("aoIntensity", aoIntensity);
    giPass->SetVector2("screenSize", {spec.width, spec.height});
    giPass->SetFloat("useScreenSpaceSampling", useScreenSpaceSampling ? 1.0f : 0.0f);
    giPass->SetFloat("temporalRotation", 1.0f);
    giPass->SetFloat("backfaceLighting", backfaceLighting);
    Graphics::DrawFullScreenQuad(*giPass, Matrix4Identity);
    Graphics::EndFramebuffer();

    Graphics::BeginFramebuffer(*pong);
    Graphics::SetViewport(0, 0, spec.width, spec.height);
    giBlurPass->SetTexture("mainTex", ping, 0);
    giBlurPass->SetTexture("gDepth", deferred, -1);
    giBlurPass->SetTexture("gNormal", deferred, 0);
    giBlurPass->SetVector2("giTexelSize", {1.0f / spec.width, 1.0f / spec.height});
    giBlurPass->SetVector2("screenSize", {spec.width, spec.height});
    Graphics::DrawFullScreenQuad(*giBlurPass, Matrix4Identity);
    Graphics::EndFramebuffer();

    Framebuffer* giFull = new Framebuffer(deferred->Specification());

    Graphics::BeginFramebuffer(*giFull);
    Graphics::SetViewport(0, 0, deferred->Specification().width, deferred->Specification().height);
    giUpsamplePass->SetTexture("giLow", pong, 0);
    giUpsamplePass->SetTexture("gDepth", deferred, -1);
    giUpsamplePass->SetTexture("gNormal", deferred, 0);
    giUpsamplePass->SetFloat("farPlane", context->GetCamera().farClip);
    giUpsamplePass->SetFloat("nearPlane", context->GetCamera().nearClip);
    giUpsamplePass->SetVector2("giTexelSize", {1.0f / spec.width, 1.0f / spec.height});
    giUpsamplePass->SetVector2("screenSize", {deferred->Specification().width, deferred->Specification().height});
    Graphics::DrawFullScreenQuad(*giUpsamplePass, Matrix4Identity);
    Graphics::EndFramebuffer();

    giComposePass->SetVector2("giSize", {deferred->Specification().width, deferred->Specification().height});
    giComposePass->SetVector2("screenSize", {deferred->Specification().width, deferred->Specification().height});
    Graphics::BeginFramebuffer(*dst);
    Graphics::SetViewport(0, 0, deferred->Specification().width, deferred->Specification().height);
    giComposePass->SetTexture("mainTex", src, 0);
    giComposePass->SetTexture("giAO", giFull, 0);
    Graphics::DrawFullScreenQuad(*giComposePass, Matrix4Identity);
    Graphics::EndFramebuffer();

    delete normal;
    delete lighting;
    delete ping;
    delete pong;
    delete giFull;
    */
}

}