#pragma once
#include "PostFX.h"
#include "OD/Serialization/SerializationCore.h"
#include "OD/Serialization/SerializationMath.h"

namespace OD{

class Framebuffer;
class Material;
class Texture2D;

//TODO: Add blur
class OD_API SSAOPostFX: public PostFX{
public:
    float intensity = 0.5f;
    float radius = 0.5f;
    float bias = 0.025f;

    SSAOPostFX();
    void OnRenderImage(Framebuffer* src, Framebuffer* dst, RenderContext* context) override;

    void OnGui() override;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(enable));
        ArchiveDump(ar, CEREAL_NVP(intensity));
        ArchiveDump(ar, CEREAL_NVP(radius));
        ArchiveDump(ar, CEREAL_NVP(bias));
    }

private:
    Ref<Material> aoPass;
    Ref<Texture2D> noise;
    std::vector<glm::vec4> ssaoKernel;
};

};