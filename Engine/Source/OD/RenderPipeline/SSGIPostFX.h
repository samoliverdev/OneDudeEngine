#pragma once
#include "PostFX.h"
#include "OD/Serialization/SerializationCore.h"
#include "OD/Serialization/SerializationMath.h"

namespace OD{

class Material;
class Framebuffer;
class Texture2D;

class OD_API SSGIPostFX: public PostFX{
public:
    SSGIPostFX();
    ~SSGIPostFX();
    void OnRenderImage(Framebuffer* src, Framebuffer* dst, RenderContext* context) override;

    void OnGui() override;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(enable));
        
        ArchiveDump(ar, CEREAL_NVP(sampleRadius));
        ArchiveDump(ar, CEREAL_NVP(hitThickness));
        ArchiveDump(ar, CEREAL_NVP(sampleCount));
        ArchiveDump(ar, CEREAL_NVP(sliceCount));
        ArchiveDump(ar, CEREAL_NVP(backfaceLighting));
        ArchiveDump(ar, CEREAL_NVP(giIntensity));
        ArchiveDump(ar, CEREAL_NVP(aoIntensity));
        ArchiveDump(ar, CEREAL_NVP(useScreenSpaceSampling));
        ArchiveDump(ar, CEREAL_NVP(denoiseMaxIterations));
        ArchiveDump(ar, CEREAL_NVP(debug));
    }

private:
    Ref<Material> giPass;
    Ref<Material> giBlurPass;
    Ref<Material> giBlurPass2;
    Ref<Material> giComposePass;
    Ref<Material> blitPass;
    Ref<Material> giUpsamplePass;
    Ref<Texture2D> blueNoise;

    class Framebuffer* lastIndirect = nullptr;

    float sampleRadius = 1;
    float hitThickness = 0.5f;
    float sampleCount = 4;
    float sliceCount = 4;
    float backfaceLighting = 0;
    float giIntensity = 1;
    float aoIntensity = 1;
    bool useScreenSpaceSampling = true; 

    int denoiseMaxIterations = 4;
    bool debug = false;
    
};

};