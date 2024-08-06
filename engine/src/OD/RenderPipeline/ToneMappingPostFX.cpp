#include "ToneMappingPostFX.h"

namespace OD{

ToneMappingPostFX::ToneMappingPostFX(){
    copyPass = Shader::CreateFromFile("res/Engine/Shaders/Blit.glsl");
    toneMappingReinhardPass = Shader::CreateFromFile("res/Engine/Shaders/ToneMappingReinhardPostFX.glsl");
    toneMappingNeutralPass = Shader::CreateFromFile("res/Engine/Shaders/ToneMappingNeutralPostFX.glsl");
    toneMappingACESPass = Shader::CreateFromFile("res/Engine/Shaders/ToneMappingACESPostFX.glsl");
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