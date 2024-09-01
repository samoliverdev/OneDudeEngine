#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "RenderContext.h"

namespace OD{
    
class OD_API BloomPostFX: public PostFX{
public:
    int maxIterations = 3;
    int downscaleLimit = 3;
    bool bicubicUpsampling = true;
    float threshold = 0.5f;
    float thresholdKnee = 1;
    float intensity = 1.0f;

    BloomPostFX();
    void OnSetup() override;
    void OnRenderImage(Framebuffer* src, Framebuffer* dst) override;

    inline void OnGui() override {
        cereal::ImGuiArchive colorGradring;
        colorGradring(*this);
    }

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