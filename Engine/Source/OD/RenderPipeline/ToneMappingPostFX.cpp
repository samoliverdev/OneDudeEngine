#include "OD/pch.h"
#include "ToneMappingPostFX.h"
#include "OD/Base.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Serialization/CerealImGui.h"
#include "RenderContext.h"
#include <magic_enum/magic_enum.hpp>

namespace OD{

void ToneMappingPostFX::OnGui() {
    cereal::ImGuiArchive colorGradring;
    colorGradring(*this);
}

ToneMappingPostFX::ToneMappingPostFX(){
    enable = false;
    copyPass = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Blit.glsl"));
    toneMappingReinhardPass = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/ToneMappingReinhardPostFX.glsl"));
    toneMappingNeutralPass = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/ToneMappingNeutralPostFX.glsl"));
    toneMappingACESPass = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/ToneMappingACESPostFX.glsl"));
}

void ToneMappingPostFX::OnRenderImage(Framebuffer* src, Framebuffer* dst, RenderContext* context){
    Ref<Material> pass = copyPass;
    if(mode == Mode::Neutral) pass = toneMappingNeutralPass;
    if(mode == Mode::Reinhard) pass = toneMappingReinhardPass;
    if(mode == Mode::ACES) pass = toneMappingACESPass;
        
    if(mode == Mode::Reinhard){
        toneMappingReinhardPass->SetFloat("exposure", exposure);
    }

    //Graphics::DrawQuadPostProcessing(src, dst, *pass);
    Graphics::BeginFramebuffer(*dst);
    pass->SetTexture("mainTex", src, 0);
    Graphics::DrawFullScreenQuad(*pass, Matrix4Identity);
    Graphics::EndFramebuffer();
}

}