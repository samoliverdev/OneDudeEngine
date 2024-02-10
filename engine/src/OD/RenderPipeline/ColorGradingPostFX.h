#pragma once

#include "OD/Serialization/Serialization.h"
#include "RenderContext.h"

namespace OD{

class ColorGradingPostFX: public PostFX{
public:
    ColorGradingPostFX(){
        _ppShader = Shader::CreateFromFile("res/Engine/Shaders/GamaCorrectionPP.glsl");
        Assert(_ppShader != nullptr);
    }

    void OnRenderImage(Framebuffer* src, Framebuffer* dst) override {
        Shader::Bind(*_ppShader);
        Graphics::BlitQuadPostProcessing(src, dst, *_ppShader);
    }

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(enable));
    }

private:
    Ref<Shader> _ppShader;
};

}