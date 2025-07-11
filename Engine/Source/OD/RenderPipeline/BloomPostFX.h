#pragma once
#include "OD/Defines.h"
#include "PostFX.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Serialization/CerealImGui.h"
#include "OD/Graphics/Framebuffer.h"
namespace OD{
    
class Material;

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
    void OnRenderImage(class Framebuffer* src, class Framebuffer* dst, class RenderContext* context) override;
    void OnGui() override;

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

        // This test works
        /*if constexpr (std::is_same_v<Archive, cereal::ImGuiArchive>){
            LogInfo("Test: %d", ar.test);
        }*/
    }

private:
    Ref<Material> blitShader;
    Ref<Material> bloomHorizontalPassShader;
    Ref<Material> bloomVerticalPassShader;
    Ref<Material> bloomCombinePassShader;
    Ref<Material> bloomPrefilterPassShader;
};

}