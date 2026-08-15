#pragma once
#include "RendererFeature.h"
#include "OD/Serialization/Serialization.h"

namespace OD{

class Framebuffer;
class Material;
class Texture2D;

//TODO: Add blur
class OD_API SSAOFeature: public RendererFeatureBase<SSAOFeature>, RenderPass{
public:
    float intensity = 0.5f;
    float radius = 0.5f;
    float bias = 0.025f;

    SSAOFeature();
    void AddRenderPasses(IRenderer& renderer, RenderContext& context) override;
    void Execute(Scene& scene, RenderContext& context, RenderFrameData& data) override;

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