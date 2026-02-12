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

void SSGIPostFX::OnGui(){
    cereal::ImGuiArchive gui;
    gui(*this);
}

SSGIPostFX::SSGIPostFX(){
    enable = false;
    
    blitPass = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Blit.glsl"));
    giPass = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/SSGIPostFX2.glsl"));
    giBlurPass = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/SSGIBlurPostFX.glsl"));
    giComposePass = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/SSGIComposePostFX.glsl"));
    giUpsamplePass = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/SSGIUpsample.glsl"));
    blueNoise = AssetManager::Get().LoadAsset<Texture2D>("Engine/Textures/LDR_RG01_47.png");
}

SSGIPostFX::~SSGIPostFX(){
    if(lastIndirect != nullptr) delete lastIndirect;
}

void SSGIPostFX::OnRenderImage(Framebuffer* src, Framebuffer* dst, RenderContext* context){
    if(context->isDeferred == false){
        Graphics::BlitFramebuffer(src, dst);
        return;
    }

    Framebuffer* deferred = context->GetDeferredFramebuffer();
    auto spec = src->Specification();

    if(lastIndirect == nullptr) lastIndirect = new Framebuffer(spec);
    lastIndirect->Resize(spec.width, spec.height);

    /*Graphics::BeginFramebuffer(*dst);
    Graphics::SetViewport(0, 0, spec.width, spec.height);
    giPass->SetTexture("mainTex", src, 0); 
    giPass->SetTexture("gNormal", deferred, 0); 
    giPass->SetTexture("gDepth", deferred, -1);
    giPass->SetTexture("lastIndirect", lastIndirect, 0);
    giPass->SetTexture("noise", blueNoise);
    giPass->SetFloat("sampleCount", sampleCount);
    giPass->SetFloat("sampleRadius", sampleRadius);
    giPass->SetFloat("sliceCount", sliceCount);
    giPass->SetFloat("hitThickness", hitThickness);
    giPass->SetFloat("giIntensity", giIntensity);
    giPass->SetVector2("screenSize", {spec.width, spec.height});
    Graphics::DrawFullScreenQuad(*giPass, Matrix4Identity);
    Graphics::EndFramebuffer();

    Graphics::BeginFramebuffer(*lastIndirect);
    Graphics::SetViewport(0, 0, spec.width, spec.height);
    blitPass->SetTexture("mainTex", dst, 0);
    Graphics::DrawFullScreenQuad(*blitPass, Matrix4Identity);
    Graphics::EndFramebuffer();
    return;*/

    spec.width /= 1; //2;
    spec.height /= 1; //2;

    auto normal = new Framebuffer(spec);
    auto lighting = new Framebuffer(spec);
    auto ping = new Framebuffer(spec);
    auto pong = new Framebuffer(spec);

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

    /*Graphics::BeginFramebuffer(*lastIndirect);
    Graphics::SetViewport(0, 0, deferred->Specification().width, deferred->Specification().height);
    blitPass->SetTexture("mainTex", ping, 0);
    Graphics::DrawFullScreenQuad(*blitPass, Matrix4Identity);
    Graphics::EndFramebuffer();*/

    delete normal;
    delete lighting;
    delete ping;
    delete pong;
    delete giFull;
}

}