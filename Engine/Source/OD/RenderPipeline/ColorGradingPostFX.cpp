#include "ColorGradingPostFX.h"

namespace OD{

ColorGradingPostFX::ColorGradingPostFX(){
    enable = false;
    colorGradingPass = SubShader::CreateFromFile("Engine/Shaders/ColorGradingPostFX.glsl");
    Assert(colorGradingPass != nullptr);
}

void ColorGradingPostFX::OnRenderImage(Framebuffer* src, Framebuffer* dst){
    SubShader::Bind(*colorGradingPass);
    colorGradingPass->SetVector4("_ColorAdjustments", Vector4(
        math::pow(2.0f, postExposure),
        contrast * 0.01f + 1.0f,
        hueShift * (1.0f / 360.0f),
        saturation * 0.01f + 1.0f
    ));
    colorGradingPass->SetVector4("_ColorFilter", colorFilter);

    Graphics::BlitQuadPostProcessing(src, dst, *colorGradingPass);
}

}