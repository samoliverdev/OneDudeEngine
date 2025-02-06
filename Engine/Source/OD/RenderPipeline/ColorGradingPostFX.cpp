#include "ColorGradingPostFX.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/Material.h"
#include "OD/Graphics/Shader.h"

namespace OD{

ColorGradingPostFX::ColorGradingPostFX(){
    enable = false;
    colorGradingPass = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/ColorGradingPostFX.glsl"));
    Assert(colorGradingPass != nullptr);
}

void ColorGradingPostFX::OnRenderImage(Framebuffer* src, Framebuffer* dst){
    colorGradingPass->SetVector4("_ColorAdjustments", Vector4(
        math::pow(2.0f, postExposure),
        contrast * 0.01f + 1.0f,
        hueShift * (1.0f / 360.0f),
        saturation * 0.01f + 1.0f
    ));
    colorGradingPass->SetVector4("_ColorFilter", colorFilter);
    Graphics::DrawQuadPostProcessing(src, dst, *colorGradingPass);
}

}