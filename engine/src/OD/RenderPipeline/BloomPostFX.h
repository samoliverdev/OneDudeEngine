#pragma once
#include "OD/Serialization/Serialization.h"
#include "RenderContext.h"

namespace OD{
    
class BloomPostFX: public PostFX{
public:
    int maxIterations = 5;
    int downscaleLimit = 5;
    bool bicubicUpsampling;
    float threshold = 0.5f;
    float thresholdKnee = 0.5f;
    float intensity = 1.0f;

    BloomPostFX();
    void OnSetup() override;
    void OnRenderImage(Framebuffer* src, Framebuffer* dst) override;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(enable));
        ArchiveDump(ar, CEREAL_NVP(maxIterations));
        ArchiveDump(ar, CEREAL_NVP(downscaleLimit));
        ArchiveDump(ar, CEREAL_NVP(bicubicUpsampling));
        ArchiveDump(ar, CEREAL_NVP(threshold));
        ArchiveDump(ar, CEREAL_NVP(thresholdKnee));
        ArchiveDump(ar, CEREAL_NVP(intensity));

        if(threshold < 0) threshold = 0;
        thresholdKnee = math::clamp<float>(thresholdKnee, 0, 1);
        if(intensity < 0) intensity = 0;
    }

private:
    Ref<Shader> blitShader;
    Ref<Shader> bloomHorizontalPassShader;
    Ref<Shader> bloomVerticalPassShader;
    Ref<Shader> bloomCombinePassShader;
    Ref<Shader> bloomPrefilterPassShader;
};

}