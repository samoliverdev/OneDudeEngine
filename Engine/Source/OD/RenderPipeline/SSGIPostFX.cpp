#include "SSGIPostFX.h"
#include "RenderContext.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Core/Application.h"

namespace OD{

SSGIPostFX::SSGIPostFX(){
    enable = false;
    giPass = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/SSGIPostFX.glsl"));
    Assert(giPass != nullptr);
}

void SSGIPostFX::OnRenderImage(Framebuffer* src, Framebuffer* dst, RenderContext* context){
    if(context->isDeferred == false){
        Graphics::BlitFramebuffer(src, dst);
        return;
    }

    Framebuffer* deferred = context->GetDeferredFramebuffer();

    giPass->SetTexture("gPosition", deferred, 0);
    giPass->SetTexture("gNormal", deferred, 1);
    giPass->SetTexture("gAlbedoSpec", deferred, 2);
    giPass->SetTexture("gEmission", deferred, 3);
    giPass->SetTexture("gOther", deferred, 4);
    giPass->SetTexture("gDepth", deferred, -1);

    giPass->SetFloat("sampleCount", sampleCount);
    giPass->SetFloat("sampleRadius", sampleRadius);
    giPass->SetFloat("sliceCount", sliceCount);
    giPass->SetFloat("hitThickness", hitThickness);
    giPass->SetVector2("screenSize", {Application::ScreenWidth(), Application::ScreenHeight()});

    Graphics::BeginFramebuffer(*dst);
    giPass->SetTexture("mainTex", src, 0);
    Graphics::DrawFullScreenQuad(*giPass, Matrix4Identity);
    Graphics::EndFramebuffer();
}

}