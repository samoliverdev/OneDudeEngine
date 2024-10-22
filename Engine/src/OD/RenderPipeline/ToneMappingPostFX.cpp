#include "ToneMappingPostFX.h"

namespace OD{

ToneMappingPostFX::ToneMappingPostFX(){
    enable = false;
    copyPass = Shader::CreateFromFile("Engine/Shaders/Blit.glsl");
    toneMappingReinhardPass = Shader::CreateFromFile("Engine/Shaders/ToneMappingReinhardPostFX.glsl");
    toneMappingNeutralPass = Shader::CreateFromFile("Engine/Shaders/ToneMappingNeutralPostFX.glsl");
    toneMappingACESPass = Shader::CreateFromFile("Engine/Shaders/ToneMappingACESPostFX.glsl");
}

void ToneMappingPostFX::OnRenderImage(Framebuffer* src, Framebuffer* dst){
    Ref<Shader> pass = copyPass;
    if(mode == Mode::Neutral) pass = toneMappingNeutralPass;
    if(mode == Mode::Reinhard) pass = toneMappingReinhardPass;
    if(mode == Mode::ACES) pass = toneMappingACESPass;
        
    if(mode == Mode::Reinhard){
        Shader::Bind(*toneMappingReinhardPass);
        toneMappingReinhardPass->SetFloat("exposure", exposure);
    }

    Graphics::BlitQuadPostProcessing(src, dst, *pass);
}

}