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
    giPass = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/SSGIPostFX4.glsl"));
    giBlurPass = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/SSGIBlurPostFX4.glsl"));
    giComposePass = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/SSGIComposePostFX.glsl"));
    giUpsamplePass = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/SSGIUpsample.glsl"));
    blueNoise = ResourceManager::Get().LoadByPath<Texture2D>("Engine/Textures/LDR_RG01_47.png");

    giBlitPass = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/SSGIBlitPostFX.glsl"));
    giTemporalFilterPass = ResourceManager::Get().Create<Material>(Shader::CreateFromFile("Engine/Shaders/SSGITemporalFilter.glsl"));
}

SSGIFeature::~SSGIFeature(){
    //if(lastIndirect != nullptr) delete lastIndirect;

    //if(giHistory != nullptr) delete giHistory;
    //if(depthHistory != nullptr) delete depthHistory;
}

void SSGIFeature::AddRenderPasses(IRenderer& renderer, RenderContext& context){
    renderer.AddPass(this);
}

void SSGIFeature::Execute(Scene& scene, RenderContext& context, RenderFrameData& data){
    if(context.isDeferred == false){
        Graphics::BlitFramebuffer(data.src.get(), data.dst.get());
        return;
    }

    auto Blit = [](Ref<Framebuffer> _src, Ref<Framebuffer> _dst, Ref<Material> blitMat, int pass = 0){
        Graphics::BeginFramebuffer(*_dst);
        Graphics::SetViewport(0, 0, _dst->Specification().width, _dst->Specification().height);
        blitMat->SetPass(pass);
        blitMat->SetTexture("mainTex", _src, 0);
        Graphics::DrawFullScreenQuad(*blitMat, Matrix4Identity);
        Graphics::EndFramebuffer();
    };

    ///*
    Ref<Framebuffer> deferred = context.GetDeferredFramebuffer();
    auto spec = data.src->Specification();
    spec.colorAttachments[0].colorFormat = FramebufferTextureFormat::RGBA16F;
    spec.createDepth = false;

    //bool useTemporalDenoise = true;
    //bool useSpatialDenoise = true;
    bool useDownSample = resolutionMode != ResolutionMode::Full;

    auto halfSpec = spec;
    if(useDownSample){
        halfSpec.width /= resolutionMode == ResolutionMode::Half ? 2 : 4;
        halfSpec.height /= resolutionMode == ResolutionMode::Half ? 2 : 4;
    }
    
    if(giFinal == nullptr) giFinal = ResourceManager::Get().Create<Framebuffer>(spec);
    giFinal->Resize(spec.width, spec.height);

    //auto gi = new Framebuffer(halfSpec);
    if(giA == nullptr) giA = ResourceManager::Get().Create<Framebuffer>(halfSpec);
    if(giB == nullptr) giB = ResourceManager::Get().Create<Framebuffer>(halfSpec);
    if(lowNormalDepth == nullptr) lowNormalDepth = ResourceManager::Get().Create<Framebuffer>(halfSpec);

    giA->Resize(halfSpec.width, halfSpec.height);
    giB->Resize(halfSpec.width, halfSpec.height);
    lowNormalDepth->Resize(halfSpec.width, halfSpec.height);

    auto GetCurGIA = [&]() -> Ref<Framebuffer> { return giStep == false ? giA : giB; }; 
    auto GetCurGIB = [&]() -> Ref<Framebuffer> { return giStep == false ? giB : giA; }; 

    if(useTemporalDenoise){
        if(giHistory == nullptr) giHistory = ResourceManager::Get().Create<Framebuffer>(halfSpec);
        if(depthHistory == nullptr) depthHistory = ResourceManager::Get().Create<Framebuffer>(spec);

        giHistory->Resize(halfSpec.width, halfSpec.height);
        depthHistory->Resize(spec.width, spec.height);
    }

    Camera cam = context.GetCamera();
    float Deg2Rad = (math::pi<float>() * 2.0f) / 360.0f;
    //float halfProjScale = spec.height / ( math::tan(cam.fov * Deg2Rad * 0.5 ) * 2 ) * 0.5;
    float halfProjScale = spec.height / (2.0f *  math::tan(math::degrees(cam.fov) * 0.5f * Deg2Rad));

    Graphics::BeginFramebuffer(*GetCurGIA());
    Graphics::SetViewport(0, 0, halfSpec.width, halfSpec.height);
    giPass->SetTexture("mainTex", data.src, 0); //giPass->SetTexture("mainTex", lighting, 0); //giPass->SetTexture("mainTex", src, 0);
    giPass->SetTexture("gNormal", deferred, 0); //giPass->SetTexture("gNormal", deferred, 1);
    giPass->SetTexture("gDepth", deferred, -1);
    giPass->SetTexture("gAlbedoSpec", deferred, 1);
    //giPass->SetTexture("lastIndirect", lastIndirect.get(), 0); //giPass->SetTexture("gNormal", deferred, 1);
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

    float rotations[8] = {60.0f, 300, 180, 240, 120, 0};
    float offsets[8] = {0, 0.5f, 0.25f, 0.75f};
    float temporalDirection = rotations[frameIndex % 6] / 360.0f;
    float temporalOffset = offsets[frameIndex % 4];

    giPass->SetFloat("_Radius", sampleRadius);
    giPass->SetFloat("_GIIntensity", giIntensity);
    giPass->SetFloat("_AOIntensity", aoIntensity);
    giPass->SetFloat("_Thickness", hitThickness);
    giPass->SetFloat("_ExpFactor", 2);
    giPass->SetFloat("_BackfaceLighting", backfaceLighting);
    giPass->SetInt("_StepCount", sampleCount);
    giPass->SetInt("_SliceCount", sliceCount);
    giPass->SetFloat("_UseLinearThickness", useLinearThickness ? 1.0f : 0.0f);
    giPass->SetFloat("_UseScreenSpaceSampling", useScreenSpaceSampling ? 1.0f : 0.0f);

    if(useTemporalDenoise){
        giPass->SetFloat("_TemporalDirection", temporalDirection);
        giPass->SetFloat("_TemporalOffset", temporalOffset);
    } else {
        giPass->SetFloat("_TemporalDirection", 1);
        giPass->SetFloat("_TemporalOffset", 1);
    }

    Graphics::DrawFullScreenQuad(*giPass, Matrix4Identity);
    Graphics::EndFramebuffer();

    if(useDownSample){
        giUpsamplePass->SetTexture("gDepth", deferred, -1);
        giUpsamplePass->SetTexture("gNormal", deferred, 0);
        giUpsamplePass->SetFloat("nearPlane", cam.nearClip);
        giUpsamplePass->SetFloat("farPlane", cam.farClip);
        giUpsamplePass->SetVector2("screenSize", {deferred->Specification().width, deferred->Specification().height});
        giUpsamplePass->SetVector2("giSize", {halfSpec.width, halfSpec.height});
        giUpsamplePass->SetVector2("giTexelSize", {1.0f / halfSpec.width, 1.0f / halfSpec.height});

        Graphics::BeginFramebuffer(*lowNormalDepth);
        Graphics::SetViewport(0, 0, halfSpec.width, halfSpec.height);
        giUpsamplePass->SetPass(0);
        giUpsamplePass->SetFloat("normalSigma", 1);
        Graphics::DrawFullScreenQuad(*giUpsamplePass, Matrix4Identity);
        Graphics::EndFramebuffer();
    }

    if(useTemporalDenoise){
        giTemporalFilterPass->SetTexture("gDepth", deferred, -1);
        giTemporalFilterPass->SetTexture("giAO", GetCurGIA(), 0);
        giTemporalFilterPass->SetTexture("gDepthHistory", depthHistory, 0);
        giTemporalFilterPass->SetTexture("giAOHistory", giHistory, 0);
        giTemporalFilterPass->SetMatrix4("lastProj", lastProj);
        giTemporalFilterPass->SetMatrix4("lastView", lastView);
        giTemporalFilterPass->SetMatrix4("lastInvProj", lastInvProj);
        giTemporalFilterPass->SetMatrix4("lastInvView", lastInvView);
        Blit(GetCurGIA(), GetCurGIB(), giTemporalFilterPass, 0);

        giBlitPass->SetTexture("gDepth", deferred, -1);
        giBlitPass->SetTexture("giAO", GetCurGIB(), 0);
        Blit(data.src, giHistory, giBlitPass, 0);
        Blit(data.src, depthHistory, giBlitPass, 1);

        giStep = !giStep;

        //Blit(giTAA.get(), gi.get(), blitPass, 0);
    }
    
    giBlurPass->SetTexture("gDepth", deferred, -1);
    giBlurPass->SetTexture("gNormal", deferred, 0);
    giBlurPass->SetTexture("gAlbedoSpec", deferred, 1);
    giBlurPass->SetFloat("cameraNear", cam.nearClip);
    giBlurPass->SetFloat("cameraFar", cam.farClip);
    giBlurPass->SetFloat("depthPhi", 100.0f*1);
    giBlurPass->SetFloat("normalPhi", 32.0f*1);

    /*
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
    */

    auto RenderAtrous = [&](Ref<Framebuffer> _src, Ref<Framebuffer> _dst, float atrousStep){
        giBlurPass->SetFloat("atrousStep", atrousStep);
        giBlurPass->SetVector2("giSize", {halfSpec.width, halfSpec.height});
        giBlurPass->SetVector2("screenSize", {spec.width, spec.height});
        Blit(_src, _dst, giBlurPass, 0);
    };

    if(useSpatialDenoise){
        RenderAtrous(GetCurGIA(), GetCurGIB(), 1);
        RenderAtrous(GetCurGIB(), GetCurGIA(), 2);

        if(denoiseMaxIterations == 2){
            RenderAtrous(GetCurGIA(), GetCurGIB(), 4);
            giStep = !giStep;
        } else if(denoiseMaxIterations > 2){
            RenderAtrous(GetCurGIA(), GetCurGIB(), 4);
            RenderAtrous(GetCurGIB(), GetCurGIA(), 8);
        }
    }

    if(useDownSample){
        Graphics::BeginFramebuffer(*giFinal);
        Graphics::SetViewport(0, 0, deferred->Specification().width, deferred->Specification().height);
        giUpsamplePass->SetPass(1);
        giUpsamplePass->SetTexture("giLow", GetCurGIA(), 0);
        giUpsamplePass->SetTexture("giSurface", lowNormalDepth, 0);
        giUpsamplePass->SetFloat("spatialSigma", 1.0f);
        giUpsamplePass->SetFloat("depthSigma", 5.0f);
        giUpsamplePass->SetFloat("normalPower", 8.0f);
        Graphics::DrawFullScreenQuad(*giUpsamplePass, Matrix4Identity);
        Graphics::EndFramebuffer();
    }
    
    giComposePass->SetPass(debug ? 1 : 0);
    giComposePass->SetVector2("giSize", {(float)deferred->Specification().width, (float)deferred->Specification().height});
    giComposePass->SetVector2("screenSize", {(float)deferred->Specification().width, (float)deferred->Specification().height});
    Graphics::BeginFramebuffer(*data.dst);
    Graphics::SetViewport(0, 0, deferred->Specification().width, deferred->Specification().height);
    giComposePass->SetTexture("mainTex", data.src, 0);
    giComposePass->SetTexture("gAlbedoSpec", deferred, 1);
    giComposePass->SetTexture("giAO", useDownSample ? giFinal : GetCurGIA(), 0);
    Graphics::DrawFullScreenQuad(*giComposePass, Matrix4Identity);
    Graphics::EndFramebuffer();

    //for(Framebuffer* cur: releaseTemporary) delete cur;

    //delete gi;

    lastProj = context.GetCamera().projection;
    lastView = context.GetCamera().view;
    lastInvProj = math::inverse(context.GetCamera().projection);
    lastInvView = math::inverse(context.GetCamera().view);
    frameIndex++;

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