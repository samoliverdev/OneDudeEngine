#include "ToneMappingPostFX.h"
#include "OD/Graphics/Graphics.h"

namespace OD{

ToneMappingPostFX::ToneMappingPostFX(){
    enable = false;
    copyPass = SubShader::CreateFromFile("Engine/Shaders/Blit.glsl");
    toneMappingReinhardPass = SubShader::CreateFromFile("Engine/Shaders/ToneMappingReinhardPostFX.glsl");
    toneMappingNeutralPass = SubShader::CreateFromFile("Engine/Shaders/ToneMappingNeutralPostFX.glsl");
    toneMappingACESPass = SubShader::CreateFromFile("Engine/Shaders/ToneMappingACESPostFX.glsl");
}

void ToneMappingPostFX::OnRenderImage(Framebuffer* src, Framebuffer* dst){
    Ref<SubShader> pass = copyPass;
    if(mode == Mode::Neutral) pass = toneMappingNeutralPass;
    if(mode == Mode::Reinhard) pass = toneMappingReinhardPass;
    if(mode == Mode::ACES) pass = toneMappingACESPass;
        
    if(mode == Mode::Reinhard){
        SubShader::Bind(*toneMappingReinhardPass);
        toneMappingReinhardPass->SetFloat("exposure", exposure);
    }

    Graphics::BlitQuadPostProcessing(src, dst, *pass);
}

}