#include "ToneMappingPostFX.h"
#include "OD/Graphics/Graphics.h"

namespace OD{

ToneMappingPostFX::ToneMappingPostFX(){
    enable = false;
    copyPass = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Blit.glsl"));
    toneMappingReinhardPass = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/ToneMappingReinhardPostFX.glsl"));
    toneMappingNeutralPass = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/ToneMappingNeutralPostFX.glsl"));
    toneMappingACESPass = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/ToneMappingACESPostFX.glsl"));
}

void ToneMappingPostFX::OnRenderImage(Framebuffer* src, Framebuffer* dst){
    Ref<Material> pass = copyPass;
    if(mode == Mode::Neutral) pass = toneMappingNeutralPass;
    if(mode == Mode::Reinhard) pass = toneMappingReinhardPass;
    if(mode == Mode::ACES) pass = toneMappingACESPass;
        
    if(mode == Mode::Reinhard){
        toneMappingReinhardPass->SetFloat("exposure", exposure);
    }

    Graphics::DrawQuadPostProcessing(src, dst, *pass);
}

}