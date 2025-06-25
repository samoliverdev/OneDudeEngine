#include "SSGIPostFX.h"
#include "RenderContext.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Core/Application.h"

namespace OD{

SSGIPostFX::SSGIPostFX(){
    enable = false;
    
    blitPass = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Blit.glsl"));
    giPass = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/SSGIPostFX.glsl"));
    giBlurPass = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/BloomHorizontalPostFX.glsl"));
    giBlurPass2 = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/BloomVerticalPostFX.glsl"));
    giComposePass = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/SSGIComposePostFX.glsl"));
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
    spec.width /= 2;
    spec.height /= 2;

    auto normal = new Framebuffer(spec);
    auto lighting = new Framebuffer(spec);
    auto pos = new Framebuffer(spec);

    auto ping = new Framebuffer(spec);
    //auto pong = new Framebuffer(spec);

    if(lastIndirect == nullptr) lastIndirect = new Framebuffer(spec);
    lastIndirect->Resize(spec.width, spec.height);

    /*Graphics::BlitFramebuffer(deferred, normal, 1);
    Graphics::BlitFramebuffer(deferred, pos, 0);
    Graphics::BlitFramebuffer(src, lighting, 0);*/
    
    Graphics::BeginFramebuffer(*normal);
    Graphics::SetViewport(0, 0, spec.width, spec.height);
    blitPass->SetTexture("mainTex", deferred, 1);
    Graphics::DrawFullScreenQuad(*blitPass, Matrix4Identity);
    Graphics::EndFramebuffer();

    Graphics::BeginFramebuffer(*pos);
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
    giPass->SetTexture("mainTex", lighting, 0); //giPass->SetTexture("mainTex", src, 0);
    giPass->SetTexture("gPosition", pos, 0); //giPass->SetTexture("gPosition", deferred, 0);
    giPass->SetTexture("gNormal", normal, 0); //giPass->SetTexture("gNormal", deferred, 1);
    giPass->SetTexture("lastIndirect", lastIndirect, 0); //giPass->SetTexture("gNormal", deferred, 1);
    giPass->SetTexture("noise", blueNoise);
    giPass->SetFloat("sampleCount", sampleCount);
    giPass->SetFloat("sampleRadius", sampleRadius);
    giPass->SetFloat("sliceCount", sliceCount);
    giPass->SetFloat("hitThickness", hitThickness);
    giPass->SetVector2("screenSize", {spec.width, spec.height});
    Graphics::DrawFullScreenQuad(*giPass, Matrix4Identity);
    Graphics::EndFramebuffer();

    Graphics::BeginFramebuffer(*lastIndirect);
    Graphics::SetViewport(0, 0, spec.width, spec.height);
    blitPass->SetTexture("mainTex", ping, 0);
    Graphics::DrawFullScreenQuad(*blitPass, Matrix4Identity);
    Graphics::EndFramebuffer();

    /*Graphics::BeginFramebuffer(*ping);
    Graphics::SetViewport(0, 0, spec.width, spec.height);
    giBlurPass->SetTexture("mainTex", lastIndirect, 0);
    Graphics::DrawFullScreenQuad(*giBlurPass, Matrix4Identity);
    Graphics::EndFramebuffer();

    Graphics::BeginFramebuffer(*pong);
    Graphics::SetViewport(0, 0, spec.width, spec.height);
    giBlurPass2->SetTexture("mainTex", ping, 0);
    Graphics::DrawFullScreenQuad(*giBlurPass2, Matrix4Identity);
    Graphics::EndFramebuffer();*/

    Graphics::BeginFramebuffer(*dst);
    Graphics::SetViewport(0, 0, deferred->Specification().width, deferred->Specification().height);
    giComposePass->SetTexture("mainTex", src, 0);
    giComposePass->SetTexture("giAO", ping, 0);
    Graphics::DrawFullScreenQuad(*giComposePass, Matrix4Identity);
    Graphics::EndFramebuffer();

    /*Graphics::BeginFramebuffer(*dst);
    Graphics::SetViewport(0, 0, deferred->Specification().width, deferred->Specification().height);
    Graphics::DrawFullScreenQuad(*giPass, Matrix4Identity);
    Graphics::EndFramebuffer();*/

    delete normal;
    delete lighting;
    delete pos;
    delete ping;
    //delete pong;
}

}